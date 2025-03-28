package service

import (
	"backend/services/go/internal/db"
	"backend/services/go/internal/server"
	"fmt"
	"net/http"
	"os"
	"path/filepath"
)

type CallbackAllow[T any] struct {
	Callback FuncPathHandler[T]
	Allow []string
}

type FuncPathHandler[T any] func(*Service[T], *server.HTTPRequest) *server.HTTPResponse
type PathMap[T any] map[string]CallbackAllow[T]

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

func allowMethod(methods []string, method string) bool {
	for _, v := range methods {
		if method == v {
			return true
		}
	}
	return false
}

func (s* Service[T]) Run() error {
	for {
		data, err := s.Recv()	
		if err != nil {
			return err
		}

		var urlPath string = data.Path
		var path CallbackAllow[T] = s.paths[urlPath]

		if allowMethod(path.Allow, data.Method) {
			resp := path.Callback(s, data)
			if resp != nil {
				s.server.Send(resp)
			}
		} else {
			s.server.Send(server.NewResponse(data, http.StatusMethodNotAllowed, nil))
		}
	}
}