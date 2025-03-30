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

type CallbackAllow[T any] struct {
	Callback FuncPathHandler[T]
	Allow []string
}

type FuncPathHandler[T any] func(*Service[T], *mq.HTTPRequest) *mq.HTTPResponse
type PathMap[T any] map[string]CallbackAllow[T]

type Service[T any] struct {
	Mq mq.MQ
	Db db.DB
	name string
	UserData T
	paths PathMap[T]
}

func New[T any](userData T) (*Service[T], error) {
	var err error
	service := Service[T]{UserData: userData}

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

func (s* Service[T]) Register(paths PathMap[T]) error {
	for path, route := range paths {
		s.Mq.BindHTTP(path, route.Allow, func (req* mq.HTTPRequest) (*mq.HTTPResponse) {
			return route.Callback(s, req)
		})
	}
	return nil
}

func (s* Service[T]) Run() {
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	sig := <-sigChan
	fmt.Printf("\n%s received signal: %s. Exiting.\n", s.name, sig)
	os.Exit(0)
}