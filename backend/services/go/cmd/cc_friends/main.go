package main

import (
	"backend/services/go/internal/server"
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"log"
	"net/http"
	"os"

	"github.com/jackc/pgx/v5/pgconn"
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
	var username string = req.Body["username"].(string)
	var userID uint32

	row := s.Db.Conn.QueryRow(context.Background(), "SELECT user_id FROM Users WHERE username = $1::text;", username)
	err := row.Scan(&userID)
	if err != nil {
		return server.NewResponse(req, http.StatusNotFound, &map[string]interface{}{
			"error": "Username not found",
		})
	}

	if userID == req.UserID {
		return server.NewResponse(req, http.StatusConflict, &map[string]interface{}{
			"error": "Can't send friend request to yourself",
		})
	}

	_, err = s.Db.Conn.Exec(context.Background(), 
							"INSERT INTO Friendships(target_user_id, source_user_id) VALUES($1::int, $2::int);", 
							userID, req.UserID)
	if err != nil {
		pgErr := err.(*pgconn.PgError)
		// duplicate
		if pgErr.Code == "23505" {
			return server.NewResponse(req, http.StatusConflict, &map[string]interface{}{
				"error": "Already friends, pending or blocked",
			})
		} else {
			return server.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
				"error": "Failed to create friend request",
			})
		}
	}

	return server.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_id": userID,
	}) 
}

func getFriendRequests(s* service.Service[FriendsData], req* server.HTTPRequest) *server.HTTPResponse {
	var userID uint32 = req.UserID

	rows, err := s.Db.Conn.Query(context.Background(), 
								"SELECT source_user_id FROM Friendships WHERE target_user_id = $1::int AND status = 'PENDING';", 
								userID)
	if err != nil {
		log.Fatalf("%v: %v\n", req.Path, err)
		return server.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var userIDs []uint32 = make([]uint32, 0)
	for rows.Next() {
		var sourceUserID uint32
		rows.Scan(&sourceUserID)

		userIDs = append(userIDs, sourceUserID)
	}

	return server.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_ids": userIDs,
	})
}

func main() {
	bservice, err := service.New(FriendsData{})
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	err = bservice.Register(service.PathMap[FriendsData]{
		"/api/friends": service.CallbackAllow[FriendsData]{
			Callback: getFriends, 
			Allow: []string{"GET"},
		},
		"/api/friend-request": service.CallbackAllow[FriendsData]{
			Callback: friendRequest,
			Allow: []string{"POST"},
		},
		"/api/friend-requests": service.CallbackAllow[FriendsData]{
			Callback: getFriendRequests,
			Allow: []string{"GET"},
		},
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
