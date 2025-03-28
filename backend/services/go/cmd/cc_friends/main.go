package main

import (
	"backend/services/go/internal/server"
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"os"
	"net/http"
)

type FriendsData struct {
	sql_select_friends string
}

func getFriends(s* service.Service[FriendsData], req* server.HTTPRequest) *server.HTTPResponse {
	fmt.Println("friends(): ", req)

	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.sql_select_friends, req.UserID)
	if err != nil {
		fmt.Printf("Query: %v\n", err)
		return server.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var friendIDs []uint32 = make([]uint32, 0)

	for rows.Next() {
		var userID uint32
		rows.Scan(&userID)
		friendIDs = append(friendIDs, userID)
	}

	return server.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"friends": friendIDs,
	})
}

func friendRequest(s* service.Service[FriendsData], req* server.HTTPRequest) *server.HTTPResponse {
	return nil
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
