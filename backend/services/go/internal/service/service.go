package service

import (
	"backend/services/go/internal/db"
	"backend/services/go/internal/mq"
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"os/signal"
	"path/filepath"
	"reflect"
	"strconv"
	"strings"
	"syscall"
)

type CallbackAllow struct {
	Callback FuncPathHandler
	Allow []string
}

type FuncPathHandler func(*Service, *mq.HTTPRequest) *mq.HTTPResponse
type PathMap map[string]CallbackAllow
type CmdMap map[string]func(*Service, uint32, map[string]any) error

type Service struct {
	Mq mq.MQ
	Db db.DB
	name string
	paths PathMap
}

func setupLog(name string) {
	log.SetPrefix(name + ": ")
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)
}

func New() (*Service, error) {
	var err error
	service := Service{}

	exePath, err := os.Executable()
	if err != nil {
		return nil, err
	}
	service.name = filepath.Base(exePath)
	setupLog(service.name)

	service.Mq, err = mq.New(service.name)
	if err != nil {
		return nil, err
	}

	service.Db, err = db.New()
	if err != nil {
		return nil, err
	}

	return &service, nil
}

func (s* Service) Register(paths PathMap) error {
	for path, route := range paths {
		s.Mq.BindHTTP(path, route.Allow, func (req* mq.HTTPRequest) (*mq.HTTPResponse) {
			return route.Callback(s, req)
		})
	}
	return nil
}

// WebSocket Command Register
func (s* Service) WSCmdRegister(cmds CmdMap) error {
	for cmd, callback := range cmds {
		s.Mq.BindWSCMD(cmd, func(srcUserID uint32, payload map[string]any) error {
			return callback(s, srcUserID, payload)
		})
	}
	return nil
}

func (s* Service) Run() {
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	sig := <-sigChan
	fmt.Printf("\n%s received signal: %s. Exiting.\n", s.name, sig)
	os.Exit(0)
}

func validateRequiredFields[T any](s T) error {
	val := reflect.ValueOf(s)
	typ := val.Type()

	for i := range val.NumField() {
		field := val.Field(i)
		fieldType := typ.Field(i)

		if _, ok := fieldType.Tag.Lookup("json"); 
			ok && strings.Contains(fieldType.Tag.Get("json"), "omitempty") {
			continue
		}

		if reflect.DeepEqual(field.Interface(), reflect.Zero(field.Type()).Interface()) {
			return fmt.Errorf("required field '%s' is missing or zero", fieldType.Name)
		}
	}

	return nil
}

func MapToStruct[T any](m map[string]any) (*T, error) {
	jsonData, err := json.Marshal(m)
	if err != nil {
		return nil, fmt.Errorf("failed to marhal map: %v", err)
	}

	var result T
	decoder := json.NewDecoder(bytes.NewReader(jsonData))
	decoder.DisallowUnknownFields()

	if err := decoder.Decode(&result); err != nil {
		return nil, fmt.Errorf("failed to decode into struct: %v", err)
	}

	if err := validateRequiredFields(result); err != nil {
		return nil, err
	}

	return &result, nil
}

func GetParamUint32(params map[string]string, name string, default_val uint32) uint32 {
	ret := default_val
	str, ok := params[name]
	if ok {
		num, err := strconv.ParseUint(str, 10, 32)
		if err == nil {
			ret = uint32(num)
		}
	}
	return ret
}
