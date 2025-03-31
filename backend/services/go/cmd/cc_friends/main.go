package main

import (
	"backend/services/go/internal/cc_friends"
	"backend/services/go/internal/service"
	"os"
	"fmt"
)



func main() {
	bservice, err := service.New(cc_friends.FriendsData{})
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_friends.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}

	bservice.UserData.Sql_select_friends = string(data)

	err = bservice.Register(service.PathMap[cc_friends.FriendsData]{
		"/api/friends": service.CallbackAllow[cc_friends.FriendsData]{
			Callback: cc_friends.GetFriends, 
			Allow: []string{"GET"},
		},
		"/api/friends/requests": service.CallbackAllow[cc_friends.FriendsData]{
			Callback: cc_friends.FriendRequests,
			Allow: []string{"GET", "POST"},
		},
		"/api/friend-request": service.CallbackAllow[cc_friends.FriendsData]{
			Callback: cc_friends.FriendRequest,
			Allow: []string{"POST"},
		},
		"/api/friends/requests/outgoing": service.CallbackAllow[cc_friends.FriendsData]{
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
