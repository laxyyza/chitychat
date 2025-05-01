package main

import (
	"backend/services/go/internal/cc_groups"
	"backend/services/go/internal/service"
	"os"
	"fmt"
)

func main() {
	bservice, err := service.New()
	if err != nil {
		fmt.Printf("Creating service: %v\n", err);
		os.Exit(-1)
	}

	err = bservice.Db.LoadSQLFiles([]string{
		"select_user_groups",
		"select_invalid_user_ids",
		"insert_group_msg",
		"select_group",
		"select_group_msgs_json",
		"delete_group",
		"add_group_members",
		"delete_group_member",
	})
	if err != nil {
		os.Exit(-1)
	}

	err = bservice.Register(service.PathMap{
		"/api/groups": service.CallbackAllow{
			Callback: cc_groups.Groups,
			Allow: []string{"GET", "POST"},
		},
		"/api/groups/*": service.CallbackAllow{
			Callback: cc_groups.SingleGroup,
			Allow: []string{"GET", "DELETE"},
		},
		"/api/groups/*/messages": service.CallbackAllow{
			Callback: cc_groups.GetMessages,
			Allow: []string{"GET"},
		},
		"/api/groups/*/members": service.CallbackAllow{
			Callback: cc_groups.AddMembers,
			Allow: []string{"POST"},
		},
		"/api/groups/*/members/me": service.CallbackAllow{
			Callback: cc_groups.DelMemberME,
			Allow: []string{"DELETE"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	err = bservice.WSCmdRegister(service.CmdMap{
		"msg_group": cc_groups.MsgGroup,
	})

	bservice.Run()
}
