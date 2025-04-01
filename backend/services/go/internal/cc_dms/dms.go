package cc_dms

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/jackc/pgx/v5/pgtype"
)

type DMs struct {
	SqlSelectDMs string
	SqlInsertMsg string
}

type Message struct {
	MsgID 		uint32		`json:"msg_id"`
	UserID 		uint32		`json:"user_id"`
	ChannelID 	uint32		`jnon:"channel_id"`
	ChannelType string 		`json:"channel_type"`
	Content 	string		`json:"content"`
	Timestamp 	string		`json:"timestamp"`
	Attachments []string	`json:"attachments"`
	ParentMsgID uint32		`json:"parent_msg_id"`
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

func MsgUser(s* service.Service[DMs], srcUserID uint32, payload map[string]any) error {
	content := payload["content"].(string)
	targetUserID := uint32(payload["user_id"].(float64))
	var msg Message = Message{UserID: srcUserID, Content: content, Attachments: []string{}, ChannelType: "DM"}

	row := s.Db.Conn.QueryRow(context.Background(),
				"SELECT channel_id FROM DirectMessages WHERE LEAST(user1_id, user2_id) = LEAST($1::int, $2::int) AND GREATEST(user1_id, user2_id) = GREATEST($1::int, $2::int);",
					srcUserID, targetUserID)

	err := row.Scan(&msg.ChannelID)
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
			srcUserID, msg.ChannelID, content, msg.Attachments)
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
