package mq

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strings"

	"github.com/nats-io/nats.go"
)

type HTTPRequest struct {
	Method 	string 					`json:"method"`
	Fd 		int 					`json:"fd"`
	UserID 	uint32 					`json:"user_id"`
	Path 	string 					`json:"path"`
	Headers map[string]string 		`json:"headers"`
	Params  map[string]string 		`json:"params"`
	Body	map[string]any 			`json:"body"`
}

type HTTPResponse struct {
	Type 	string 					`json:"type"`
	Fd 		int 					`json:"fd"`
	Status 	int 					`json:"status"`
	Headers map[string]string 		`json:"headers"`
	Body*	map[string]any 			`json:"body,omitempty"`
}

type WSRequest struct {
	UserID 	uint32 			`json:"user_id"`
	Payload map[string]any 	`json:"payload"`
}

type SubCallbackType func (*HTTPRequest) (*HTTPResponse)
type WSCallbackType func (uint32, map[string]any) error

type MQ struct {
	conn* nats.Conn
	httpQueueName string
	wsQueueName string
}

func toRequest(m* nats.Msg) (*HTTPRequest) {
	var req HTTPRequest
	err := json.Unmarshal(m.Data, &req)
	if err != nil {
		log.Printf("json.Unmarshal failed: %s\n", err)
		return nil
	}

	return &req
}

func New(queueName string) (MQ, error) {
	var mq MQ = MQ{httpQueueName: queueName + ".http", wsQueueName: queueName + ".ws"}
	var err error

	natsUrl := os.Getenv("NATS_URL")
	if natsUrl == "" {
		natsUrl = nats.DefaultURL
	}

	mq.conn, err = nats.Connect(natsUrl)
	if err != nil {
		log.Printf("Failed to connect to NATS server: %s\n", err)
		return MQ{}, err
	}

	return mq, err
}

func (mq* MQ) BindHTTP(endpoint string, methods []string, callback SubCallbackType) {
	var natsEndpoint = "http" + strings.ReplaceAll(endpoint, "/", ".")

	for _, method := range methods {
		var subject string = natsEndpoint + "." + method
		log.Printf("SUB %s\n", subject)

		mq.conn.QueueSubscribe(subject, mq.httpQueueName, func (m* nats.Msg) {
			req := toRequest(m)
			if req == nil {
				return
			}

			resp := callback(req)
			if resp == nil {
				return
			}

			data, err := json.Marshal(resp)
			if err != nil {
				log.Printf("json.Marshal failed: %s\n", err)
				return
			}
			m.Respond(data)
		})
	}
}

func (mq* MQ) BindWSCMD(cmd string, callback WSCallbackType) {
	var natsSubject = "ws.cmd." + cmd

	log.Printf("SUB %s\n", natsSubject)
	mq.conn.QueueSubscribe(natsSubject, mq.wsQueueName, func(m* nats.Msg) {
		var req WSRequest
		err := json.Unmarshal(m.Data, &req)
		if err != nil {
			log.Printf("ws: json.Unmarshal: %s\n", err)
			return
		}

		callback(req.UserID, req.Payload)
	})
}

func NewResponse(req* HTTPRequest, status int, body* map[string]any) *HTTPResponse {
	if body != nil {
		return &HTTPResponse{
			Type: req.Method, 
			Fd: req.Fd, 
			Status: status, 
			Headers: map[string]string{
				"Content-Type": "application/json",
			},
			Body: body,
		}
	} else {
		return &HTTPResponse{
			Type: req.Method,
			Fd: req.Fd,
			Status: status,
		}
	}
}

func (mq* MQ) UserEvent(userID uint32, event map[string]interface{}) {
	data, err := json.Marshal(event)
	if err != nil {
		log.Printf("RealTimeEvent json.Marshal failed: %s\n", err)
		return
	}

	mq.conn.Publish(fmt.Sprintf("realtime.user.%d", userID), data)
}

func (mq* MQ) UsersEvent(userIDs []uint32, event map[string]interface{}) {
	for _, userID := range userIDs {
		mq.UserEvent(userID, event)
	}
}
