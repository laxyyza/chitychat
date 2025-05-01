package service

import (
	"backend/services/go/internal/db" 
	"backend/services/go/internal/mq"
	"fmt"
	"os"
	"os/signal"
	"syscall"
	"path/filepath"
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

func New() (*Service, error) {
	var err error
	service := Service{}

	exePath, err := os.Executable()
	if err != nil {
		return nil, err
	}
	service.name = filepath.Base(exePath)

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
