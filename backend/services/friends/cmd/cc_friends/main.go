package main

import (
	"cc_friends/internal/server"
	"fmt"
)

func main() {
	conn, err := server.Connect("localhost:6000")
	if err != nil {
		fmt.Println("Connect: %w", err)
		return
	}
	defer conn.Close()

	err = server.Send(conn, map[string]interface{}{
		"register_paths": [...]string{"/friends"},
		"name": "cc_friends",
	})
	if err != nil {
		fmt.Println("Send register: %w", err)
		return
	}

	resp, err := server.Recv(conn)	
	if err != nil {
		return
	}
	fmt.Println("Resp: %w", resp)
	
	for {
		resp, err := server.Recv(conn)	
		if err != nil {
			return
		}

		friend_ids := [...]uint32{69, 420, 21, 666}

		fmt.Println("Resp: %w", resp)
		err = server.Send(conn, map[string]interface{}{
			"type": resp["type"],
			"fd": resp["fd"],
			"status": 200,
			"headers": map[string]interface{}{
				"Content-Type": "application/json",
			},
			"payload": map[string]interface{}{
				"friends": friend_ids,
			},
		})
	}
}