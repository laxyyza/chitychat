package main

import (
	"backend/services/go/internal/mq"
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

// Get friends
func getFriends(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	fmt.Println("friends(): ", req)

	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.sql_select_friends, req.UserID)
	if err != nil {
		fmt.Printf("Query: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var friendIDs []uint32 = make([]uint32, 0)

	for rows.Next() {
		var userID uint32
		rows.Scan(&userID)
		friendIDs = append(friendIDs, userID)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"friends": friendIDs,
	})
}

// Send friend request 
func friendRequest(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	var username string = req.Body["username"].(string)
	var userID uint32

	row := s.Db.Conn.QueryRow(context.Background(), "SELECT user_id FROM Users WHERE username = $1::text;", username)
	err := row.Scan(&userID)
	if err != nil {
		return mq.NewResponse(req, http.StatusNotFound, &map[string]interface{}{
			"error": "Username not found",
		})
	}

	if userID == req.UserID {
		return mq.NewResponse(req, http.StatusConflict, &map[string]interface{}{
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
			return mq.NewResponse(req, http.StatusConflict, &map[string]interface{}{
				"error": "Already friends, pending or blocked",
			})
		} else {
			return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
				"error": "Failed to create friend request",
			})
		}
	}

	s.Mq.UserEvent(userID, map[string]interface{}{
		"cmd": "friend_request",
		"source_user_id": req.UserID,
	})

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_id": userID,
	})
}

func actionFriendRequest(s* service.Service[FriendsData], req* mq.HTTPRequest, status string) *mq.HTTPResponse {
	sourceUserID := req.UserID

	targetUserIDf64, ok := req.Body["user_id"].(float64)
	if (!ok) {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"status": "error",
			"error": "user_id",
		})
	}
	targetUserID := uint32(targetUserIDf64)
	var friendShipID uint32

	row := s.Db.Conn.QueryRow(context.Background(), 
								"SELECT friendship_id FROM Friendships WHERE target_user_id = $1::int AND source_user_id = $2::int AND status = 'PENDING';",
								sourceUserID, targetUserID)
	err := row.Scan(&friendShipID)	
	if (err != nil) {
		fmt.Printf("Query friendship id: %s\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"status": "error",
			"error": "Cant find friend request",
		})
	}

	_, err = s.Db.Conn.Exec(context.Background(), 
							"UPDATE Friendships SET status = $1::friendship_status WHERE friendship_id = $2::int;", 
							status, friendShipID)
	if (err != nil) {
		fmt.Printf("Failed to update friendship: %s\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	s.Mq.UserEvent(sourceUserID, map[string]interface{}{
		"cmd": "friend_request_update",
		"user_id": targetUserID,
		"state": status,
	})
	s.Mq.UserEvent(targetUserID, map[string]interface{}{
		"cmd": "friend_request_update",
		"user_id": sourceUserID,
		"state": status,
	})

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"status": "success",
	})
}

func postFriendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	fmt.Println(req)
	switch action := req.Body["action"]; action {
	case "accept": 
		return actionFriendRequest(s, req, "ACCEPTED")
	case "block":
		return actionFriendRequest(s, req, "BLOCKED")
	default:
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"error": "Only 'accept' or 'block' actions are allowed",
		})
	}
}

func getFriendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	var userID uint32 = req.UserID

	rows, err := s.Db.Conn.Query(context.Background(),
								"SELECT source_user_id FROM Friendships WHERE target_user_id = $1::int AND status = 'PENDING';",
								userID)
	if err != nil {
		log.Fatalf("%v: %v\n", req.Path, err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var userIDs []uint32 = make([]uint32, 0)
	for rows.Next() {
		var sourceUserID uint32
		rows.Scan(&sourceUserID)

		userIDs = append(userIDs, sourceUserID)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_ids": userIDs,
	})
}

// Get friend requests.
func friendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	if (req.Method == "GET") {
		return getFriendRequests(s, req)
	} else if (req.Method == "POST") {
		return postFriendRequests(s, req)
	} else {
		return nil
	}
}

// Get outgoing friend requests.
func getOutgoingFriendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	sourceUserID := req.UserID

	rows, err := s.Db.Conn.Query(context.Background(), 
								"SELECT target_user_id FROM Friendships WHERE source_user_id = $1::int AND status = 'PENDING';",
								sourceUserID)								
	if err != nil {
		fmt.Printf("Get outgoing friend requests failed: %s\n",
					err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var userIDs []uint32 = make([]uint32, 0)
	for rows.Next() {
		var targetUserID uint32
		rows.Scan(&targetUserID)

		userIDs = append(userIDs, targetUserID)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_ids": userIDs,
	})
}

func deleteOutgoingFriendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	sourceUserID := req.UserID
	targetUserIDf64 := req.Body["user_id"].(float64)
	targetUserID := uint32(targetUserIDf64)

	_, err := s.Db.Conn.Exec(context.Background(), 
								"DELETE FROM Friendships WHERE source_user_id = $1::int AND target_user_id = $2::int AND status = 'PENDING';",
								sourceUserID, targetUserID)
	if err != nil {
		fmt.Printf("Failed to delete outgoing friend request: %s\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"status": "error",
			"error": "Failed to delete friend request",
		})
	}

	s.Mq.UserEvent(targetUserID, map[string]interface{}{
		"cmd": "friend_request_update",
		"user_id": sourceUserID,
		"state": "DELETED",
	})
	s.Mq.UserEvent(sourceUserID, map[string]interface{}{
		"cmd": "friend_request_update",
		"user_id": targetUserID,
		"state": "DELETED",
	})

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"status": "success",
	})
}

func outgoingFriendRequests(s* service.Service[FriendsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	if req.Method == "GET" {
		return getOutgoingFriendRequests(s, req)
	} else if (req.Method == "DELETE") {
		return deleteOutgoingFriendRequests(s, req)
	} else {
		return nil
	}
}

func main() {
	bservice, err := service.New(FriendsData{})
	if err != nil {
		fmt.Printf("Creating service: %v\n", err)
		os.Exit(-1)
	}

	data, err := os.ReadFile("backend/sql/select_friends.sql")
	if err != nil {
		fmt.Printf("ReadFile: %v\n", err)
		os.Exit(-1)
	}

	bservice.UserData.sql_select_friends = string(data)

	err = bservice.Register(service.PathMap[FriendsData]{
		"/api/friends": service.CallbackAllow[FriendsData]{
			Callback: getFriends, 
			Allow: []string{"GET"},
		},
		"/api/friends/requests": service.CallbackAllow[FriendsData]{
			Callback: friendRequests,
			Allow: []string{"GET", "POST"},
		},
		"/api/friend-request": service.CallbackAllow[FriendsData]{
			Callback: friendRequest,
			Allow: []string{"POST"},
		},
		"/api/friends/requests/outgoing": service.CallbackAllow[FriendsData]{
			Callback: outgoingFriendRequests,
			Allow: []string{"GET", "DELETE"},
		},
	})
	if err != nil {
		fmt.Printf("Register: %v\n", err)
		os.Exit(-1)
	}

	bservice.Run()
}
