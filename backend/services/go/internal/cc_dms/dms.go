package cc_dms

import (
	"backend/services/go/internal/mq"
	"backend/services/go/internal/service"
	"context"
	"fmt"
	"net/http"
)

type DMs struct {
	SqlSelectDMs string
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