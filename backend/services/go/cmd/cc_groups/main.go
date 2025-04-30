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

	data, err = os.ReadFile("backend/sql/insert_group_msg.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.InsertMessage = string(data)

	data, err = os.ReadFile("backend/sql/select_group.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SelectGroup = string(data)

	data, err = os.ReadFile("backend/sql/select_group_msgs_json.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.SelectMsgs = string(data)

	data, err = os.ReadFile("backend/sql/delete_group.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.DeleteGroup = string(data)

	data, err = os.ReadFile("backend/sql/add_group_members.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}
	bservice.UserData.AddGroupMembers = string(data)

	err = bservice.Register(service.PathMap[cc_groups.GroupsData]{
		"/api/groups": service.CallbackAllow[cc_groups.GroupsData]{
			Callback: cc_groups.Groups,
			Allow: []string{"GET", "POST"},
		},
		"/api/groups/*": service.CallbackAllow[cc_groups.GroupsData]{
			Callback: cc_groups.SingleGroup,
			Allow: []string{"GET", "DELETE"},
		},
		"/api/groups/*/messages": service.CallbackAllow[cc_groups.GroupsData]{
			Callback: cc_groups.GetMessages,
			Allow: []string{"GET"},
		},
		"/api/groups/*/members": service.CallbackAllow[cc_groups.GroupsData]{
			Callback: cc_groups.AddMembers,
			Allow: []string{"POST"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	err = bservice.WSCmdRegister(service.CmdMap[cc_groups.GroupsData]{
		"msg_group": cc_groups.MsgGroup,
	})

	bservice.Run()
}
