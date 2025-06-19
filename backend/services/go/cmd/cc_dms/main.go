package main

import (
	"backend/services/go/internal/cc_dms"
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

	bservice.Db.LoadSQLFiles([]string{
		"select_dms",
		"insert_msg",
		"select_msgs_json",
	})

	err = bservice.Register(service.PathMap{
		"/api/dms": service.CallbackAllow{
			Callback: cc_dms.GetDMs,
			Allow: []string{"GET"},
		},
		"/api/dms/*": service.CallbackAllow{
			Callback: cc_dms.GetMessages,
			Allow: []string{"GET"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %s\n", err)
		os.Exit(-1)
	}
	err = bservice.WSCmdRegister(service.CmdMap{
		"msg_user": cc_dms.MsgUser,
	})
	if err != nil {
		fmt.Printf("Failed to register WS CMDs: %s\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
