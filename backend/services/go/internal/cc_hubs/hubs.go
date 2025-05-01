package cc_hubs

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/msg"
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"strings"
	"encoding/json"

	"github.com/jackc/pgx/v5/pgtype"
)

type CreateHubRequest struct {
	Name string `json:"name"`
}

const insertHub = "INSERT INTO Hubs(owner_id, name) VALUES ($1::int, $2::text) RETURNING hub_id;"
const insertCategory = "INSERT INTO HubCategories(hub_id, name, position) VALUES ($1::int, $2::text, $3::int) RETURNING category_id;"
const insertChannel = "INSERT INTO HubChannels(hub_id, category_id, name, position) VALUES ($1::int, $2::int, $3::text, $4::int) RETURNING channel_id;"

func getHubIDPath(path string) (uint32, error) {
	split := strings.Split(path[1:], "/")
	// e.g. HTTP GET /api/hubs/69 = ["api", "hubs", "69"]
	
	if len(split) >= 3 {
		groupIDString := split[2]
		var groupID uint64

		groupID, err := strconv.ParseUint(groupIDString, 10, 32)
		return uint32(groupID), err
	} else {
		return 0, fmt.Errorf("invalid path")
	}
}

// HTTP POST /api/hubs
func createHub(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	var hubID uint32 
	var categoryID uint32 
	reqData, err := service.MapToStruct[CreateHubRequest](req.Body)
	if err != nil {
		return mq.NewResponse(req, http.StatusInternalServerError, &map[string]any{
			"error": err.Error(),
		})
	}

	tx, err := s.Db.Conn.Begin(context.Background())
	if err != nil {
		log.Printf("BEGIN: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	row := tx.QueryRow(context.Background(), insertHub, req.UserID, reqData.Name)
	err = row.Scan(&hubID)
	if err != nil {
		log.Printf("INSERT HUB: %v\n", err)
		tx.Rollback(context.Background())
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	row = tx.QueryRow(context.Background(), insertCategory, hubID, "TEXT CHANNELS", 0)
	err = row.Scan(&categoryID)
	if err != nil {
		log.Printf("INSERT CATEGORY: %v\n", err)
		tx.Rollback(context.Background())
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	_, err = tx.Exec(context.Background(), insertChannel, hubID, categoryID, "General", 0)
	if err != nil {
		log.Printf("INSERT CHANNEL: %v\n", err)
		tx.Rollback(context.Background())
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	err = tx.Commit(context.Background())
	if err != nil {
		log.Printf("COMMIT: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"hub_id": hubID, 
	})
}

// HTTP GET /api/hubs
// - Get basic info about hubs user are in.
func getHubs(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	var hubs []any
	row := s.Db.QueryRow("select_user_hubs_json", req.UserID)
	err := row.Scan(&hubs)
	if err != nil {
		log.Printf("select_user_hubs_json: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"hubs": hubs,
	})
}

func Hubs(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	switch req.Method {
	case "POST":
		return createHub(s, req)
	case "GET":
		return getHubs(s, req)
	default:
		return nil
	}
}

// HTTP GET /api/hubs/:hub_id
// -- Gets detailed info about that specific hub.
func GetHub(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	hubID, err := getHubIDPath(req.Path)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	var hub map[string]any
	row := s.Db.QueryRow("select_hub_detailed_json", hubID, req.UserID)
	if err := row.Scan(&hub); err != nil {
		log.Printf("select_hub_detailed_json (hub_id: %d): %v\n", hubID, err)
		return mq.NewResponse(req, http.StatusBadRequest, nil)
	}

	return mq.NewResponse(req, http.StatusOK, &hub)
}

func mapGetUint32(payload map[string]any, fieldName string) (uint32, error) {
	uint32f64, ok := payload[fieldName].(float64)
	if !ok {
		return 0, fmt.Errorf("invalid '%s'", fieldName)
	}
	return uint32(uint32f64), nil
}

func checkPermissions(s* service.Service, userID uint32, hubID uint32, channelID uint32) error {
	var roles any // User ID roles.
	var channelSettings any // Channel Settings.
	row := s.Db.QueryRow("select_roles_channel_id", hubID, userID, channelID)
	if err := row.Scan(&roles, &channelSettings); err != nil {
		return err
	}

	// TODO: if roles and channel settings are implemented.

	return nil
}

func broadcastMsg(s* service.Service, message msg.Message, hubID uint32) {
	rows, err := s.Db.Conn.Query(context.Background(), "SELECT user_id FROM HubMembers WHERE hub_id = $1::int;", hubID)
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
	eventCMD["hub_id"] = hubID
	
	delete(eventCMD, "channel_type")

	for rows.Next() {
		var memberID uint32
		rows.Scan(&memberID)

		s.Mq.UserEvent(memberID, eventCMD)
	}
}

func MsgHub(s* service.Service, srcUserID uint32, payload map[string]any) error {
	message, err := msg.FromUser(srcUserID, payload, msg.Hub)
	if err != nil {
		log.Printf("MsgHub msg.FromUser: %v\n", err)
		return nil
	}
	hubID, err := mapGetUint32(payload, "hub_id")
	if err != nil {
		log.Printf("MsgHub: %v\n", err)
		return nil
	}
	if err := checkPermissions(s, srcUserID, hubID, message.ChannelID); err != nil {
		log.Printf("checkPermissions for USER_ID=%d HUB_ID=%d CHANNEL_ID=%d: %v\n",
			srcUserID, hubID, message.ChannelID, err)
		return nil
	}

	var timestamp pgtype.Timestamp
	row := s.Db.QueryRow("insert_hub_msg", srcUserID, message.ChannelID, message.Content)
	if err := row.Scan(&message.MsgID, &timestamp); err != nil {
		log.Printf("Ainsert_hub_msg: %v\n", err)
		return nil
	}
	message.Timestamp = timestamp.Time.String()

	broadcastMsg(s, message, hubID)

	return nil
}
