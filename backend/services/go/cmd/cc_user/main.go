package main

import (
	"backend/services/go/internal/cc_user"
	"backend/services/go/internal/service"
	"fmt"
	"os"
)

func main() {
	bservice, err := service.New()
	if err != nil {
		fmt.Printf("Failed to create service: %s\n", err)
		os.Exit(-1)
	}

	bservice.Db.LoadSQLFiles([]string{})

	err = bservice.Register(service.PathMap{
		"/api/users/me": service.CallbackAllow{
			Callback: cc_user.PatchUserME,
			Allow: []string{"PATCH"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %s\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
