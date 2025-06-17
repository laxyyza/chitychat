package cc_user

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"encoding/json"
	"log"
	"net/http"
)

const updateUsernameSQL = "UPDATE Users SET username = $1::text WHERE user_id = $2::int;"
const updateDisplayNameSQL = "UPDATE Users SET displayname = $1::text WHERE user_id = $2::int;"
const updatePfpUrlSQL = "UPDATE Users SET pfp_url = $1::text WHERE user_id = $2::int;"
const updateAboutMeSQL = "UPDATE Users SET bio = $1::text WHERE user_id = $2::int;"

type PatchData struct {
	Username 	*string	`json:"username,omitempty"`
	DisplayName *string	`json:"displayname,omitempty"`
	PfpURL 		*string	`json:"pfp_url,omitempty"`
	AboutME		*string	`json:"about_me,omitempty"`
}

func structToMap(input any) *map[string]any {
	data, err := json.Marshal(input)
	if err != nil {
		return nil
	}

	var ret map[string]any
	_ = json.Unmarshal(data, &ret)
	return &ret
}

func PatchUserME(s* service.Service, req* mq.HTTPRequest) *mq.HTTPResponse {
	if len(req.Body) == 0 {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": "Missing one required field",
		})
	}

	patch, err := service.MapToStruct[PatchData](req.Body)
	if err != nil {
		return mq.NewResponse(req, http.StatusBadRequest, &map[string]any{
			"error": err.Error(),
		})
	}
	userID := req.UserID

	tx, err := s.Db.Conn.Begin(context.Background())
	if err != nil {
		log.Printf("BEGIN: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	if patch.Username != nil {
		log.Println("UPADTE USERNAME!")

		_, err = tx.Exec(context.Background(), updateUsernameSQL, patch.Username, userID)
		if err != nil {
			log.Printf("Update Username: %v\n", err)
			tx.Rollback(context.Background())
			return mq.NewResponse(req, http.StatusConflict, nil)
		}
	}

	if patch.DisplayName != nil {
		log.Println("UPDATE DISPLAY NAME")

		_, err = tx.Exec(context.Background(), updateDisplayNameSQL, patch.DisplayName, userID)
		if err != nil {
			log.Printf("Update Display Name: %v\n", err)
			tx.Rollback(context.Background())
			return mq.NewResponse(req, http.StatusInternalServerError, nil)
		}
	}

	if patch.PfpURL != nil {
		log.Println("UPDATE PFP URL")

		_, err = tx.Exec(context.Background(), updatePfpUrlSQL, patch.PfpURL, userID)
		if err != nil {
			log.Printf("Update PFP URL: %v\n", err)
			tx.Rollback(context.Background())
			return mq.NewResponse(req, http.StatusInternalServerError, nil)
		}
	}

	if patch.AboutME != nil {
		log.Println("UPDATE ABOUT ME")

		_, err = tx.Exec(context.Background(), updateAboutMeSQL, patch.AboutME, userID)
		if err != nil {
			log.Printf("Update About ME: %v\n", err)
			tx.Rollback(context.Background())
			return mq.NewResponse(req, http.StatusInternalServerError, nil)
		}
	}

	err = tx.Commit(context.Background())
	if err != nil {
		log.Printf("commit: %v\n", err)
		return mq.NewResponse(req, http.StatusInternalServerError, nil)
	}

	return mq.NewResponse(req, http.StatusOK, structToMap(patch))
}

