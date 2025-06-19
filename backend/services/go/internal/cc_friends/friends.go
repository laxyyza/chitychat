package cc_friends

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"log"
	"net/http"
	"github.com/jackc/pgx/v5/pgconn"
)

type UserIDStruct struct {
	UserID uint32 	`json:"user_id"`
}

// HTTP GET /api/friends
func GetFriends(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	fmt.Println("friends(): ", req)

	rows, err := s.Db.Conn.Query(context.Background(), s.Db.SQL["select_friends"], req.UserID)
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

// HTTP POST /api/friend-request
func FriendRequest(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
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

func actionFriendRequest(s* service.Service, req* mq.HTTPRequest, status string) *mq.HTTPResponse {
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

// HTTP POST /api/friends/requests
func postFriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
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

// HTTP GET /api/friends/requests
func getFriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
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

// HTTP GET|POST /api/friends/requests
func FriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	switch (req.Method) {
	case "GET":
		return getFriendRequests(s, req)
	case "POST":
		return postFriendRequests(s, req)
	default:
		return mq.NewResponse(req, http.StatusMethodNotAllowed, nil)
	}
}

// HTTP GET /api/friends/requests/outgoing
func getOutgoingFriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
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

// HTTP DELETE /api/friends/requests/outgoing
func deleteOutgoingFriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	sourceUserID := req.UserID

	data, err := service.MapToStruct[UserIDStruct](req.Body)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}
	targetUserID := data.UserID

	_, err = s.Db.Conn.Exec(context.Background(), 
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

// HTTP GET|DELETE /api/friends/requests/outgoing
func OutgoingFriendRequests(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	switch (req.Method) {
	case "GET":
		return getOutgoingFriendRequests(s, req)
	case "DELETE":
		return deleteOutgoingFriendRequests(s, req)
	default:
		return mq.NewResponse(req,http.StatusMethodNotAllowed, nil)
	}
}
