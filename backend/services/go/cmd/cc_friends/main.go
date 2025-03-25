package main

import (
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"os"
)

func main() {
	service, err := service.New()
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	err = service.Register([]string{"/friends"})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_friends.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}

	sql := string(data)

	for {
		resp, err := service.Recv()	
		if err != nil {
			return
		}
		rows, err := service.Db.Conn.Query(context.Background(), sql, resp["user_id"])
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

		err = service.Send(map[string]interface{}{
			"type": resp["type"],
			"fd": resp["fd"],
			"status": 200,
			"headers": map[string]interface{}{
				"Content-Type": "application/json",
			},
			"payload": map[string]interface{}{
				"friends": friendIDs,
			},
		})
	}
}
