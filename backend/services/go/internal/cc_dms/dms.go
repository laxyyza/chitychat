package cc_dms

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"backend/services/go/internal/msg"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgtype"
)

type DMs struct {
	SqlSelectDMs string
	SqlInsertMsg string
	SqlSelectMsgs string
}

func selectChannelID(s* service.Service[DMs], user1ID uint32, user2ID uint32) (uint32, error) {
	var channelID uint32
	row := s.Db.Conn.QueryRow(context.Background(),
				"SELECT channel_id FROM DirectMessages WHERE LEAST(user1_id, user2_id) = LEAST($1::int, $2::int) AND GREATEST(user1_id, user2_id) = GREATEST($1::int, $2::int);",
				user1ID, user2ID)

	err := row.Scan(&channelID)

	return channelID, err
}

func GetDMs(s* service.Service[DMs], req* mq.HTTPRequest) *mq.HTTPResponse {
	userID := req.UserID

	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.SqlSelectDMs, userID)
	if err != nil {
		fmt.Printf("%s %s: Failed: %s\n", req.Method, req.Path, err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	var userIDs []uint32 = make([]uint32, 0)
	for rows.Next() {
		var id uint32
		rows.Scan(&id)
		userIDs = append(userIDs, id)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"user_ids": userIDs,
	}) 
}

func getMsgPathUserID(path string) (uint32, error) {
	split := strings.Split(path[1:], "/")
	// e.g. HTTP GET /api/dms/69  =  ["api", "dms", "69"]

	if len(split) == 3 {
		userIDString := split[2]
		var userID uint64

		userID, err := strconv.ParseUint(userIDString, 10, 32)
		return uint32(userID), err
	} else {
		return 0, fmt.Errorf("invalid path")
	}
}

func selectMessages(s* service.Service[DMs], channelID uint32, limit uint32, offset uint32) ([]interface{}, error) {
	var msgsJson []interface{}
	row := s.Db.Conn.QueryRow(context.Background(), s.UserData.SqlSelectMsgs, 
		channelID, limit, offset)
	err := row.Scan(&msgsJson)

	return msgsJson, err
}

func GetMessages(s* service.Service[DMs], req* mq.HTTPRequest) *mq.HTTPResponse {
	targetUserID, err := getMsgPathUserID(req.Path)
	var limit uint32 = 10
	var offset uint32
	if err != nil {
		fmt.Printf("Failed to get target user ID: %s\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{"error": err})
	}

	channelID, err := selectChannelID(s, req.UserID, targetUserID)
	if err != nil {
		fmt.Printf("Failed to get channel ID: %s\n", err)
		return mq.NewResponse(req, http.StatusUnauthorized, nil)
	}

	limitStr, ok := req.Params["limit"]
	if ok {
		num, err := strconv.ParseUint(limitStr, 10, 32)
		if err == nil {
			limit = uint32(num)
		}
	}
	offsetStr, ok := req.Params["offset"]
	if ok {
		num, err := strconv.ParseUint(offsetStr, 10, 32)
		if err == nil {
			offset = uint32(num)
		}
	}

	msgs, err := selectMessages(s, channelID, limit, offset)
	if err != nil {
		fmt.Printf("Select Messages failed: %s\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"messages": msgs,
	}) 
}

func MsgUser(s* service.Service[DMs], srcUserID uint32, payload map[string]any) error {
	// content := payload["content"].(string)
	targetUserID := uint32(payload["user_id"].(float64))
	// var msg = msg.Message{UserID: srcUserID, Content: content, Attachments: []string{}, ChannelType: "DM"}
	msg, err := msg.FromUser(srcUserID, payload, msg.DM)
	if err != nil {
		fmt.Printf("msg.FromUser: %v\n", err)
		return nil
	}
	var row pgx.Row

	msg.ChannelID, err = selectChannelID(s, srcUserID, targetUserID)
	if err != nil {
		tx, err := s.Db.Conn.Begin(context.Background())
		if err != nil {
			fmt.Printf("Failed to begin transaction: %s\n", err)
			return nil
		}

		row = s.Db.Conn.QueryRow(context.Background(), 
			"INSERT INTO TextChannels(type) VALUES('DM') RETURNING channel_id;")
		err = row.Scan(&msg.ChannelID)
		if err != nil {
			fmt.Printf("Failed to insert text channels: %s\n", err)
			tx.Rollback(context.Background())
			return nil
		}

		_, err = s.Db.Conn.Exec(context.Background(), 
			"INSERT INTO DirectMessages(user1_id, user2_id, channel_id) VALUES (LEAST($1::int, $2::int), GREATEST($1::int, $2::int), $3::int);",
					srcUserID, targetUserID, msg.ChannelID)
		if err != nil {
			fmt.Printf("Failed to insert DM: %s\n", err)
			tx.Rollback(context.Background())
			return nil
		}
		
		tx.Commit(context.Background())
	}

	var pgTime pgtype.Timestamp
	row = s.Db.Conn.QueryRow(context.Background(), s.UserData.SqlInsertMsg, 
			srcUserID, msg.ChannelID, msg.Content, msg.Attachments)
	err = row.Scan(&msg.MsgID, &pgTime)
	if err != nil {
		fmt.Printf("Failed to insert msg: %s\n", err)
		return nil
	}
	msg.Timestamp = pgTime.Time.String()

	jsonData, _:= json.Marshal(msg)
	var eventCMD map[string]any
	_ = json.Unmarshal(jsonData, &eventCMD)
	eventCMD["cmd"] = "msg_user"

	// First send to target user. 
	s.Mq.UserEvent(targetUserID, eventCMD)

	// Second send to source user, with target user ID included.
	eventCMD["target_user_id"] = targetUserID
	s.Mq.UserEvent(srcUserID, eventCMD)

	return nil
}
