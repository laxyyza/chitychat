package main

import (
	"backend/services/go/internal/cc_friends"
	"backend/services/go/internal/service"
	"os"
	"fmt"
)



func main() {
	bservice, err := service.New()
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	bservice.Db.LoadSQLFiles([]string{
		"select_friends",
	})

	err = bservice.Register(service.PathMap{
		"/api/friends": service.CallbackAllow{
			Callback: cc_friends.GetFriends, 
			Allow: []string{"GET"},
		},
		"/api/friends/requests": service.CallbackAllow{
			Callback: cc_friends.FriendRequests,
			Allow: []string{"GET", "POST"},
		},
		"/api/friend-request": service.CallbackAllow{
			Callback: cc_friends.FriendRequest,
			Allow: []string{"POST"},
		},
		"/api/friends/requests/outgoing": service.CallbackAllow{
			Callback: cc_friends.OutgoingFriendRequests,
			Allow: []string{"GET", "DELETE"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
