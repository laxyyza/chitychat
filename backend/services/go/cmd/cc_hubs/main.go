package main

import (
	"backend/services/go/internal/cc_hubs"
	"backend/services/go/internal/service"
	"log"
)

func main() {
	bservice, err := service.New()
	if err != nil {
		log.Panic("PANIC: ", err)
	}

	err = bservice.Db.LoadSQLFiles([]string{
		"select_user_hubs_json",
		"select_hub_detailed_json",
		"select_roles_channel_id",
		"insert_hub_msg",
		"select_hub_msgs_json",
	})
	if err != nil {
		log.Panic("LoadSQLFiles: ", err)
	}

	err = bservice.Register(service.PathMap{
		"/api/hubs": service.CallbackAllow{
			Callback: cc_hubs.Hubs,
			Allow: []string{"GET", "POST"},
		},
		"/api/hubs/*": service.CallbackAllow{
			Callback: cc_hubs.GetHub,
			Allow: []string{"GET"},
		},
		"/api/hubs/*/channels/*/messages": service.CallbackAllow{
			Callback: cc_hubs.GetChannelMessages,
			Allow: []string{"GET"},
		},
	})
	if err != nil {
		log.Panic("bservice.Register: ", err)
	}
	
	if err := bservice.WSCmdRegister(service.CmdMap{
		"msg_hub": cc_hubs.MsgHub,
	}); err != nil {
		log.Panic("WSCmdRegister: ", err)
	}

	bservice.Run()
}
