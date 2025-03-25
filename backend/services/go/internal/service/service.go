package service

import (
	"backend/services/go/internal/db"
	"backend/services/go/internal/server"
	"fmt"
	"os"
	"path/filepath"
)

type Service struct {
	server server.Server
	db db.DB
	name string
}

func New() (*Service, error) {
	var err error
	service := Service{}

	exePath, err := os.Executable()
	if err != nil {
		return nil, err
	}
	service.name = filepath.Base(exePath)

	service.server, err = server.New()
	if err != nil {
		return nil, err
	}

	service.db, err = db.New()
	if err != nil {
		return nil, err
	}

	return &service, nil
}

func (s* Service) Register(paths []string) error {
	err := s.server.Send(map[string]interface{}{
		"name": s.name,
		"register_paths": paths,
	})
	if err != nil {
		return err
	}

	resp, err := s.server.Recv()
	if err != nil {
		return err
	}

	fmt.Println("register:", resp)

	return nil
}

func (s* Service) Send(data map[string]interface{}) error {
	return s.server.Send(data)
}

func (s* Service) Recv() (map[string]interface{}, error) {
	return s.server.Recv()
}