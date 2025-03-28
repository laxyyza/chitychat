package main

import (
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"os"
)

type FriendsData struct {
	sql_select_friends string
}

func getFriends(s* service.Service[FriendsData], req map[string]interface{}) {
	fmt.Println("friends(): ", req)

	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.sql_select_friends, req["user_id"])
	if err != nil {
		fmt.Printf("Query: %v\n", err)
		os.Exit(-1)
	}

	var friendIDs []uint32 = make([]uint32, 0)

	for rows.Next() {
		var userID uint32
		rows.Scan(&userID)
		friendIDs = append(friendIDs, userID)
	}

	s.Send(map[string]interface{}{
		"type": req["type"],
		"fd": req["fd"],
		"status": 200,
		"headers": map[string]interface{}{
			"Content-Type": "application/json",
		},
		"payload": map[string]interface{}{
			"friends": friendIDs,
		},
	})
}

func friendRequest(s* service.Service[FriendsData], req map[string]interface{}) {

}

func main() {
	bservice, err := service.New(FriendsData{})
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	err = bservice.Register(service.PathMap[FriendsData]{
		"/api/friends": getFriends,
		"/api/friend-request": friendRequest,
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_friends.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}

	bservice.UserData.sql_select_friends = string(data)

	bservice.Run()
}
