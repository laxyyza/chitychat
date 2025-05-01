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
	})
	if err != nil {
		log.Panic("bservice.Register: ", err)
	}

	bservice.Run()
}
