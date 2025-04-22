package main

import (
	"backend/services/go/internal/cc_groups"
	"backend/services/go/internal/service"
	"os"
	"fmt"
)

func main() {
	bservice, err := service.New(cc_groups.GroupsData{})
	if err != nil {
		fmt.Printf("Creating service: %v\n", err);
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_user_groups.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SelectUserGroups = string(data)

	data, err = os.ReadFile("backend/sql/select_invalid_user_ids.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SelectInvalidUserIDs = string(data)

	err = bservice.Register(service.PathMap[cc_groups.GroupsData]{
		"/api/groups": service.CallbackAllow[cc_groups.GroupsData]{
			Callback: cc_groups.Groups,
			Allow: []string{"GET", "POST"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
