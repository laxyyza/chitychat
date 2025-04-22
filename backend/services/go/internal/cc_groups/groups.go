package cc_groups

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgtype"
)

type GroupsData struct {
	SelectUserGroups string
	SelectInvalidUserIDs string
}

type CreateGroupData struct {
	Name string 		`json:"name"`
	Desc string 		`json:"desc"`
	UserIDs []uint32 	`json:"user_ids"`
}

type Group struct {
	GroupID uint32 		`json:"group_id"`
	OwnerID uint32 		`json:"owner_id"`
	Name 	string 		`json:"name"`
	Desc	string 		`json:"desc"`
	CreatedAt string 	`json:"created_at"`
}

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
		rows.Scan(&group.GroupID, &group.OwnerID, &group.Name, &group.Desc, &createdAt)
		group.CreatedAt = createdAt.Time.String()
		groups = append(groups, group)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]interface{}{
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

	insertGroup := "INSERT INTO Groups(owner_id, name, \"desc\") VALUES ($1::int, $2::varchar(50), $3::text) RETURNING group_id;"

	tx, err := s.Db.Conn.Begin(context.Background())
	if err != nil {
		fmt.Printf("Failed to begin! %v\n", err);
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	row := s.Db.Conn.QueryRow(context.Background(), insertGroup, ownerID, data.Name, data.Desc)
	err = row.Scan(&groupID)
	if err != nil {
		tx.Rollback(context.Background())
		fmt.Printf("insertGroup: %v\n", err)
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
