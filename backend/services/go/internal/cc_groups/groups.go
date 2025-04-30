package cc_groups

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/msg"
	"backend/services/go/internal/service"
	"context"
	"encoding/json"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgtype"
)

type GroupsData struct {
	SelectUserGroups string
	SelectGroup string
	SelectInvalidUserIDs string
	SelectMsgs string
	InsertMessage string
	DeleteGroup string
}

type CreateGroupData struct {
	Name string 		`json:"name"`
	Desc string 		`json:"desc"`
	UserIDs []uint32 	`json:"user_ids"`
}

type Group struct {
	GroupID uint32 		`json:"group_id"`
	OwnerID uint32 		`json:"owner_id"`
	ChannelID uint32 	`json:"channel_id"`
	Name 	string 		`json:"name"`
	Desc	string 		`json:"desc"`
	MemberIDs []uint32  `json:"member_ids"`
	CreatedAt string 	`json:"created_at"`
	LastMessage string 	`json:"last_message"`
}

const insertGroupSQL = "INSERT INTO Groups(owner_id, channel_id, name, \"desc\") VALUES ($1::int, $2::int, $3::varchar(50), $4::text) RETURNING group_id;"
const insertChannelSQL = "INSERT INTO TextChannels(type) VALUES ('GROUP') RETURNING channel_id;"
const selectGroupMembers = "SELECT user_id FROM GroupMembers WHERE group_id = $1::int;"

func mapToStruct(m map[string]interface{}, out* CreateGroupData) error {
    b, err := json.Marshal(m)
    if err != nil {
        return err
    }
	err = json.Unmarshal(b, out)
	if err != nil {
		return err
	}

	if out.Name == "" {
		return fmt.Errorf("invalid 'name'")
	}
	if len(out.UserIDs) == 0 {
		return fmt.Errorf("invalid 'user_ids'")
	}

	return err
}

func getGroups(s* service.Service[GroupsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.SelectUserGroups, req.UserID)
	if err != nil {
		fmt.Printf("SelectUserGroups: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
			"error": "Failed to get groups",
		})
	}

	var groups []Group = make([]Group, 0)
	for rows.Next() {
		var group Group
		var createdAt pgtype.Timestamp
		var lastMessage pgtype.Timestamp
		err = rows.Scan(&group.GroupID, &group.OwnerID, &group.ChannelID, &group.Name, &group.Desc, &createdAt, &group.MemberIDs, &lastMessage)
		if err != nil {
			fmt.Printf("rows.Scan: %v\n", err)
		}
		group.CreatedAt = createdAt.Time.String()
		group.LastMessage = lastMessage.Time.String()
		groups = append(groups, group)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"groups": groups,
	})
}

func insertGroupMembers(s* service.Service[GroupsData], req* mq.HTTPRequest, groupID uint32, userIDs []uint32) *mq.HTTPResponse {
	rows := make([][]interface{}, len(userIDs))
	for i, userID := range userIDs {
		rows[i] = []interface{}{userID, groupID}
	}

	fmt.Println(rows, userIDs)
	_, err := s.Db.Conn.CopyFrom(
		context.Background(), 
		pgx.Identifier{"groupmembers"}, 
		[]string{"user_id", "group_id"}, 
		pgx.CopyFromRows(rows))
	if err != nil {
		fmt.Printf("Failed to insert group members: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
			"error": "Failed to insert group members",
		})
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"group_id": groupID,
	})
}

func getInvalidUserIDs(s* service.Service[GroupsData], data* CreateGroupData, ownerID uint32) []uint32 {
	rows, err := s.Db.Conn.Query(context.Background(), s.UserData.SelectInvalidUserIDs, data.UserIDs)
	if err != nil {
		fmt.Printf("SelectInvalidUserIDs: %v\n", err)
		return []uint32{}
	}

	var invalidUserIDs []uint32 = make([]uint32, 0)

	for rows.Next() {
		var invalidID uint32
		rows.Scan(&invalidID)
		invalidUserIDs = append(invalidUserIDs, invalidID)
	}

	for _, id := range data.UserIDs {
		if id == ownerID {
			invalidUserIDs = append(invalidUserIDs, id)
			break
		}
	}

	return invalidUserIDs
}

func createGroup(s* service.Service[GroupsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	ownerID:= req.UserID
	var data CreateGroupData
	err := mapToStruct(req.Body, &data)
	if err != nil {
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
			"error": err.Error(),
		})
	}

	invalidUserIDs := getInvalidUserIDs(s, &data, ownerID)
	if len(invalidUserIDs) > 0 {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"error": "Invalid User IDs",
			"invalid_user_ids": invalidUserIDs,
		})
	}

	var groupID uint32
	var channelID uint32

	tx, err := s.Db.Conn.Begin(context.Background())
	if err != nil {
		fmt.Printf("Failed to begin! %v\n", err);
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	row := s.Db.Conn.QueryRow(context.Background(), insertChannelSQL)
	err = row.Scan(&channelID)
	if err != nil {
		tx.Rollback(context.Background())
		fmt.Printf("insertChannel: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
			"error": "Failed to create group",
		})
	}

	row = s.Db.Conn.QueryRow(context.Background(), insertGroupSQL, ownerID, channelID, data.Name, data.Desc)
	err = row.Scan(&groupID)
	if err != nil {
		tx.Rollback(context.Background())
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]interface{}{
			"error": "Failed to create group",
		})
	}

	resp := insertGroupMembers(s, req, groupID, data.UserIDs) 
	if resp.Status != http.StatusOK {
		tx.Rollback(context.Background())
	} else {
		tx.Commit(context.Background())
	}
	return resp
}



func Groups(s* service.Service[GroupsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	switch req.Method {
	case "GET":
		return getGroups(s, req)
	case "POST":
		return createGroup(s, req)
	default:
		return nil
	}
}

func getGroupIDPath(path string) (uint32, error) {
	split := strings.Split(path[1:], "/")
	// e.g. HTTP GET /api/groups/69 = ["api", "groups", "69"]
	
	if len(split) == 3 || len(split) == 4 {
		groupIDString := split[2]
		var groupID uint64

		groupID, err := strconv.ParseUint(groupIDString, 10, 32)
		return uint32(groupID), err
	} else {
		return 0, fmt.Errorf("invalid path")
	}
}

func getGroup(s* service.Service[GroupsData], req* mq.HTTPRequest, groupID uint32) *mq.HTTPResponse {
	var group Group
	row := s.Db.Conn.QueryRow(context.Background(), s.UserData.SelectGroup, groupID, req.UserID)
	var createdAt pgtype.Timestamp
	var lastMessage pgtype.Timestamp
	err := row.Scan(&group.GroupID, &group.OwnerID, &group.ChannelID, &group.Name, &group.Desc, &createdAt, &group.MemberIDs, &lastMessage)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"error": "Group not found",
		})
	}
	group.CreatedAt = createdAt.Time.String()
	group.LastMessage = lastMessage.Time.String()

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
		"group": group,
	})
}

func deleteGroup(s* service.Service[GroupsData], req* mq.HTTPRequest, groupID uint32) *mq.HTTPResponse {
	tag, err := s.Db.Conn.Exec(context.Background(), s.UserData.DeleteGroup, groupID, req.UserID)
	if err != nil {
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	if tag.Delete() {
		if tag.RowsAffected() == 1 {
			return mq.NewResponse(req, http.StatusOK, nil)
		}
	}

	return mq.NewResponse(req, http.StatusUnauthorized, nil)
}

func SingleGroup(s* service.Service[GroupsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	groupID, err := getGroupIDPath(req.Path)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]interface{}{
			"error": err.Error(),
		})
	}

	switch req.Method {
	case "GET":
		return getGroup(s, req, groupID);
	case "DELETE":
		return deleteGroup(s, req, groupID);
	default:
		return nil;
	}
}

func broadcastMsg(s* service.Service[GroupsData], message msg.Message, groupID uint32) {
	rows, err := s.Db.Conn.Query(context.Background(), "SELECT user_id FROM GroupMembers WHERE group_id = $1::int;", groupID)
	if err != nil {
		fmt.Printf("broadcastMsg: %v\n", err)
		return
	}

	jsonData, err := json.Marshal(&message)
	if err != nil {
		fmt.Printf("broadcastMsg json: %v\n", err)
		return
	}
	var eventCMD map[string]any
	_ = json.Unmarshal(jsonData,&eventCMD) 
	eventCMD["cmd"] = "msg_group"
	eventCMD["group_id"] = groupID
	
	delete(eventCMD, "channel_type")

	for rows.Next() {
		var memberID uint32
		rows.Scan(&memberID)

		s.Mq.UserEvent(memberID, eventCMD)
	}
}

func MsgGroup(s* service.Service[GroupsData], srcUserID uint32, payload map[string]any) error {
	msg, err := msg.FromUser(srcUserID, payload, msg.Group)
	if err != nil {
		fmt.Printf("msg.FromUser: %v\n", err)
		return nil
	}
	groupIDf64, ok := payload["group_id"].(float64)
	if ok == false {
		return nil
	}
	groupID := uint32(groupIDf64)

	row := s.Db.Conn.QueryRow(context.Background(), s.UserData.InsertMessage, msg.UserID, groupID, msg.Content)
	var timestamp pgtype.Timestamp
	err = row.Scan(&msg.MsgID, &timestamp)
	if err != nil {
		fmt.Printf("InsertMessage: %v\n", err)
		return nil
	}
	msg.Timestamp = timestamp.Time.String()

	broadcastMsg(s, msg, groupID)

	return nil
}

func GetMessages(s* service.Service[GroupsData], req* mq.HTTPRequest) *mq.HTTPResponse {
	groupID, err := getGroupIDPath(req.Path);
	if err != nil {
		fmt.Printf("getGroupIDPath: %v\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, nil)
	}
	var limit uint32 = 10
	var offset uint32 = 0

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

	var msgsJson []any
	row := s.Db.Conn.QueryRow(context.Background(), s.UserData.SelectMsgs, groupID, req.UserID, limit, offset)
	err = row.Scan(&msgsJson)
	if err != nil {
		fmt.Printf("SelectMsgs: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	if msgsJson == nil {
		msgsJson = make([]any, 0)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"messages": msgsJson,
	}) 
}
