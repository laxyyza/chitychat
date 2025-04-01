package main

import (
	"backend/services/go/internal/cc_dms"
	"backend/services/go/internal/service"
	"fmt"
	"os"
)

func main() {
	bservice, err := service.New(cc_dms.DMs{})
	if err != nil {
		fmt.Printf("Failed to create service: %s\n", err)
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_dms.sql")
	if err != nil {
		fmt.Printf("Failed to read file: %s\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SqlSelectDMs = string(data)

	data, err = os.ReadFile("backend/sql/insert_msg.sql")
	if err != nil {
		fmt.Printf("Failed to read file: %s\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SqlInsertMsg = string(data)


	err = bservice.Register(service.PathMap[cc_dms.DMs]{
		"/api/dms": service.CallbackAllow[cc_dms.DMs]{
			Callback: cc_dms.GetDMs,
			Allow: []string{"GET"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %s\n", err)
		os.Exit(-1)
	}
	err = bservice.WSCmdRegister(service.CmdMap[cc_dms.DMs]{
		"msg_user": cc_dms.MsgUser,
	})
	if err != nil {
		fmt.Printf("Failed to register WS CMDs: %s\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
