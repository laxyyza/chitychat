package service

import (
	"backend/services/go/internal/db"
	"backend/services/go/internal/server"
	"fmt"
	"os"
	"path/filepath"
)

type FuncPathHandler[T any] func(*Service[T], *server.HTTPRequest) *server.HTTPResponse
type PathMap[T any] map[string]FuncPathHandler[T]

type Service[T any] struct {
	server server.Server
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

	service.server, err = server.New()
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
	pathStr := make([]string, 0)

	for path := range paths {
		pathStr = append(pathStr, path)	
	}

	err := s.server.Send(map[string]interface{}{
		"name": s.name,
		"register_paths": pathStr,
	})
	if err != nil {
		return err
	}

	resp, err := s.server.Recv()
	if err != nil {
		return err
	}

	s.paths = paths
	fmt.Println("register:", resp)

	return nil
}

func (s* Service[T]) Send(data* server.HTTPResponse) error {
	return s.server.SendResp(data)
}

func (s* Service[T]) Recv() (*server.HTTPRequest, error) {
	return s.server.Recv()
}

func (s* Service[T]) Run() error {
	for {
		data, err := s.Recv()	
		if err != nil {
			return err
		}

		var path string = data.Path
		resp := s.paths[path](s, data)
		if resp != nil {
			s.server.Send(resp)
		}
	}
}