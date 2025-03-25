package main

import (
	"backend/services/go/internal/service"
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
	
	for {
		resp, err := service.Recv()	
		if err != nil {
			return
		}

		friend_ids := [...]uint32{69, 420, 21, 666}

		fmt.Println("Resp: %w", resp)
		err = service.Send(map[string]interface{}{
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