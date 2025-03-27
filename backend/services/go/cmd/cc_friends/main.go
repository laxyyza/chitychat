package main

import (
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"os"
)

type FriendsData struct {
	sql string
}

func getFriends(s* service.Service[FriendsData], req map[string]interface{}) {
	fmt.Println("friends(): ", req)

	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.sql, req["user_id"])
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
		"/api/friend-request": getFriends,
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

	bservice.UserData.sql = string(data)

	bservice.Run()

	// for {
	// 	resp, err := bservice.Recv()	
	// 	if err != nil {
	// 		return
	// 	}

	// 	log.Printf("resp: %v\n", resp)

	// 	if resp["path"] == "/api/friends" {
	// 		rows, err := s.Db.Conn.Query(context.Background(), sql, resp["user_id"])
	// 		if err != nil {
	// 			fmt.Printf("Query: %v\n", err)
	// 			os.Exit(-1)
	// 		}

	// 		var friendIDs []uint32 = make([]uint32, 0)

	// 		for rows.Next() {
	// 			var userID uint32
	// 			rows.Scan(&userID)
	// 			friendIDs = append(friendIDs, userID)
	// 		}

	// 		err = service.Send(map[string]interface{}{
	// 			"type": resp["type"],
	// 			"fd": resp["fd"],
	// 			"status": 200,
	// 			"headers": map[string]interface{}{
	// 				"Content-Type": "application/json",
	// 			},
	// 			"payload": map[string]interface{}{
	// 				"friends": friendIDs,
	// 			},
	// 		})
	// 	} else if resp["path"] == "/api/friend-request" {
	// 	}
	// }
}
