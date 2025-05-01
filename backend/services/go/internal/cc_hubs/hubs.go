package cc_hubs

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"log"
	"net/http"
)

const insertHub = "INSERT INTO Hubs(owner_id, name) VALUES ($1::int, $2::text) RETURNING hub_id;"
const insertCategory = "INSERT INTO HubCategories(hub_id, name, position) VALUES ($1::int, $2::text, $3::int) RETURNING category_id;"
const insertChannel = "INSERT INTO HubChannels(hub_id, category_id, name, position) VALUES ($1::int, $2::int, $3::text, $4::int) RETURNING channel_id;"

func createHub(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	var hubID uint32 
	var categoryID uint32 
	name, ok := req.Body["name"].(string)
	if !ok {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": "Invalid 'name'",
		})
	}

	tx, err := s.Db.Conn.Begin(context.Background())
	if err != nil {
		log.Printf("BEGIN: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	row := tx.QueryRow(context.Background(), insertHub, req.UserID, name)
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

func Hubs(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	switch req.Method {
	case "POST":
		return createHub(s, req)
	// case "GET":
	// 	return getHubs(s, req)
	default:
		return nil
	}
}
