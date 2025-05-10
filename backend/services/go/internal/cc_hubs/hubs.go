package cc_hubs

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/msg"
	"backend/services/go/internal/service"
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"strings"
	"time"
	"math/rand"

	"github.com/jackc/pgx/v5/pgtype"
)

type CreateHubRequest struct {
	Name string `json:"name"`
}

type HubChannel struct {
	ChannelID 	uint32 	`json:"channel_id"`
	HubID 		uint32  `json:"hub_id"`
	CategoryID 	uint32  `json:"category_id"`
	Name 		string 	`json:"name"`
	CreatedAT 	string 	`json:"created_at"`
	Settings 	any 	`json:"settings"`
	Position 	int 	`json:"position"`
}

type CreateChannelData struct {
	Name string `json:"name"`
	Position int `json:"position"`
}

type CreateInviteData struct {
	MaxUses uint32 `json:"max_uses"`
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

// Returns Hub ID and Channel ID from path 
func getHubIDChannelIDPath(path string) (uint32, uint32, error) {
	split := strings.Split(path[1:], "/")
	// e.g. HTTP GET /api/hubs/69/channels/420/messasges = ["api", "hubs", "69", "channels", "420", "messages"]
	
	if len(split) >= 5 {
		groupIDString := split[2]
		groupID, err := strconv.ParseUint(groupIDString, 10, 32)
		if err != nil {
			return 0, 0, err
		}

		channelIDString := split[4]
		channelID, err := strconv.ParseUint(channelIDString, 10, 32)

		return uint32(groupID), uint32(channelID), err
	} else {
		return 0, 0, fmt.Errorf("invalid path")
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

	if len(hubs) == 0 {
		hubs = []any{}
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

// HTTP GET /api/hubs/:hub_id/channels/:channel_id/messages
// 	params: 
//		- limit
//		- offset
func GetChannelMessages(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	limit := service.GetParamUint32(req.Params, "limit", 10)
	offset := service.GetParamUint32(req.Params, "offset", 0)
	hubID, channelID, err := getHubIDChannelIDPath(req.Path)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	var msgsJson map[string]any = make(map[string]any)
	row := s.Db.QueryRow("select_hub_msgs_json", 
		hubID, req.UserID, channelID, limit, offset)
	err = row.Scan(&msgsJson)
	if err != nil {
		log.Printf("select_hub_msgs_json: USER_ID=%d, HUB_ID=%d, CHANNEL_ID=%d: %v\n",
			req.UserID, hubID, channelID, err)
		return mq.NewResponse(req, http.StatusUnauthorized, nil)
	}
	if msgsJson["messages"] == nil {
		msgsJson["messages"] = []any{}
	}

	return mq.NewResponse(req, http.StatusOK, &msgsJson)
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
	rows, err := s.Db.Query("SELECT user_id FROM HubMembers WHERE hub_id = $1::int;", hubID)
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
	eventCMD["cmd"] = "msg_hub"
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
	message.ChannelID, err = mapGetUint32(payload, "channel_id")
	if err != nil {
		log.Printf("Invalid 'channel_id'")
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
		log.Printf("insert_hub_msg: %v\n", err)
		return nil
	}
	message.Timestamp = timestamp.Time.String()

	broadcastMsg(s, message, hubID)

	return nil
}

func broadcastEvent(s* service.Service, cmd string, payload any, hubID uint32, ignoreID uint32) {
	rows, err := s.Db.Conn.Query(context.Background(), "SELECT user_id FROM HubMembers WHERE hub_id = $1::int;", hubID)
	if err != nil {
		fmt.Printf("broadcastMsg: %v\n", err)
		return
	}

	jsonData, err := json.Marshal(&payload)
	if err != nil {
		fmt.Printf("broadcastMsg json: %v\n", err)
		return
	}
	var eventCMD map[string]any
	_ = json.Unmarshal(jsonData,&eventCMD) 
	eventCMD["cmd"] = cmd;

	for rows.Next() {
		var memberID uint32
		rows.Scan(&memberID)

		if memberID != ignoreID {
			s.Mq.UserEvent(memberID, eventCMD)
		}
	}
}

func CreateChannel(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	hubID, categoryID, err := getHubIDChannelIDPath(req.Path)
	if err != nil {
		log.Printf("CreateChannel: %v\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}
	data, err := service.MapToStruct[CreateChannelData](req.Body)
	if err != nil {
		log.Printf("CreateChannel: MapToStruct: %v\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	var c HubChannel
	var timestamp pgtype.Timestamp
	row:= s.Db.QueryRow("insert_hub_channel", 
		hubID, categoryID, data.Name, data.Position, req.UserID)
	err = row.Scan(&c.ChannelID, &c.HubID, &c.CategoryID, &c.Name, &timestamp, &c.Settings, &c.Position)
	if err != nil {
		log.Printf("insert_hub_channel: %v\n", err)
		return mq.NewResponse(req, http.StatusBadRequest, nil)
	}
	c.CreatedAT = timestamp.Time.String()

	broadcastEvent(s, "new_hub_channel", c, hubID, 0)

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"channel_id": c.ChannelID,
	})
}

func CreateCategory(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	hubID, err := getHubIDPath(req.Path)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	data, err := service.MapToStruct[CreateChannelData](req.Body)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	var categoryID uint32
	row := s.Db.QueryRow("insert_hub_category", hubID, data.Name, data.Position, req.UserID)
	err = row.Scan(&categoryID)
	if err != nil {
		log.Printf("insert_category: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	broadcastEvent(s, "new_hub_category", map[string]any{
		"category_id": categoryID,
		"hub_id": hubID,
		"name": data.Name,
		"position": data.Position,
	}, hubID, 0)

	return mq.NewResponse(req, http.StatusOK, nil)
}

func getTextPath(path string, idx int) (string, error) {
	slices := strings.Split(path, "/")

	if len(slices) < idx {
		return "", fmt.Errorf("invalid path")
	}

	return slices[idx], nil
}

func userIsHubMember(s* service.Service, userID uint32, hubID uint32) bool {
	var isMember bool

	err := s.Db.QueryRow(`
		SELECT EXISTS (
			SELECT 1 FROM HubMembers
			WHERE user_id = $1::int AND hub_id = $2::int
		);
		`, userID, hubID).Scan(&isMember);
	if err != nil {
		log.Printf("userIsHubMember: %v\n", err)
	}

	return isMember
}

// HTTP GET /api/hubs/invites/:code
func getInvite(s* service.Service, req* mq.HTTPRequest, code string) *mq.HTTPResponse {
	var hubName string 
	var hubID uint32
	var membersCount int 
	var expiresAt pgtype.Timestamp
	var username string 
	var displayname string
	var expiresAtString any
	err := s.Db.QueryRow("select_hub_invite", code).Scan(
		&hubName,
		&membersCount,
		&hubID,
		&expiresAt,
		&username,
		&displayname)
	if err != nil {
		log.Printf("select_hub_invite: CODE='%s': %v\n", code, err)
		return mq.NewResponse(req, http.StatusNotFound, nil)
	}

	if expiresAt.Valid {
		if expiresAt.Time.UTC().Before(time.Now().UTC()) {
			return mq.NewResponse(req, http.StatusGone, &map[string]any{
				"error": "Link expired",
			})
		}
		expiresAtString = expiresAt.Time.String()
	} else {
		expiresAtString = nil
	}

	isMember := userIsHubMember(s, req.UserID, hubID)

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"hub_name": hubName,
		"members_count": membersCount,
		"expired_at": expiresAtString,
		"username": username,
		"displayname": displayname,
		"already_joined": isMember,
	});
}

// HTTP POST /api/hubs/invites/:code
func postInvite(s* service.Service, req* mq.HTTPRequest, code string) *mq.HTTPResponse {
	var hubID uint32
	err := s.Db.QueryRow(`
		SELECT hub_id FROM HubInvites 
		WHERE code = $1::text;`, code).Scan(&hubID)
	if err != nil {
		log.Printf("postInvite SELECT hub_id CODE='%s': %v\n", code, err)
		return mq.NewResponse(req, http.StatusNotFound, nil)
	}

	_, err = s.Db.Exec(`
		INSERT INTO HubMembers(user_id, hub_id)
		VALUES ($1::int, $2::int);`, req.UserID, hubID)
	if err != nil {
		log.Printf("INSERT INTO HubMembers user_id=%d, hub_id=%d, code='%s': %v\n",
			req.UserID, hubID, code, err)
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": "Already member or internal error",
		})
	}

	_, err = s.Db.Exec(`
		INSERT INTO HubInviteUses(code, user_id)
		VALUES ($1::text, $2::int);`, 
		code, req.UserID)
	if err != nil {
		log.Printf("INSERT INTO HubInviteUses: user_id=%d, code='%s': %v\n", req.UserID, code, err)
	}

	broadcastEvent(s, "new_hub_member", map[string]any{
		"hub_id": hubID,
		"user_id": req.UserID,
	}, hubID, req.UserID)

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"hub_id": hubID,
	})
}

// HTTP GET|POST /api/hubs/invites/:code
func Invites(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	code, err := getTextPath(req.Path, 4)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	switch req.Method {
	case "GET":
		return getInvite(s, req, code)
	case "POST":
		return postInvite(s, req, code)
	default:
		return nil
	}
}


func generateRandomString(length int) string {
	// Define the characters to choose from
	const charset = "abcdefghijklmnopqrstuvwxyz1234567890"
	var result []byte
	rand.Seed(time.Now().UnixNano()) // Seed the random number generator

	for i := 0; i < length; i++ {
		randomIndex := rand.Intn(len(charset)) // Get a random index
		result = append(result, charset[randomIndex]) // Append the character at the index
	}

	return string(result)
}

// HTTP POST /api/hubs/:hub_id/invites
func CreateInvite(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	hubID, err := getHubIDPath(req.Path)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}

	var data CreateInviteData 
	if maxUsesf64, ok := req.Body["max_uses"].(float64); !ok {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": "Missing 'max_uses'",
		})
	} else {
		data.MaxUses = uint32(maxUsesf64)
	}

	if !userIsHubMember(s, req.UserID, hubID) {
		return mq.NewResponse(req, http.StatusForbidden, &map[string]any{
			"error": "Not a member",
		})
	}

	code := generateRandomString(6)
	maxUses := pgtype.Uint32{}
	if data.MaxUses == 0 {
		maxUses.Valid = false
	} else {
		maxUses.Uint32 = data.MaxUses
	}

	_, err = s.Db.Exec(`
		INSERT INTO HubInvites(code, hub_id, created_by, max_uses)
		VALUES ($1::text, $2::int, $3::int, $4::int);`, 
		code, hubID, req.UserID, maxUses)
	if err != nil {
		log.Printf("INSERT INTO HubInvites: code='%s', user_id=%d, hub_id=%d: %v\n",
			code, req.UserID, hubID, err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	return mq.NewResponse(req, http.StatusOK, &map[string]any{
		"code": code,
	})
}

