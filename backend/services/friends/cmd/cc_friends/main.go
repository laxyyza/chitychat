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
}