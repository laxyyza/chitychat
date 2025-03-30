package mq

import (
	"encoding/json"
	"fmt"
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
	Body	map[string]interface{} 	`json:"body"`
}

type HTTPResponse struct {
	Type 	string 					`json:"type"`
	Fd 		int 					`json:"fd"`
	Status 	int 					`json:"status"`
	Headers map[string]string 		`json:"headers"`
	Body*	map[string]interface{} 	`json:"body"`
}

type SubCallbackType func (*HTTPRequest) (*HTTPResponse)

type MQ struct {
	conn* nats.Conn
	queueName string
}

func toRequest(m* nats.Msg) (*HTTPRequest) {
	var req HTTPRequest
	err := json.Unmarshal(m.Data, &req)
	if err != nil {
		fmt.Printf("json.Unmarshal failed: %s\n", err)
		return nil
	}

	return &req
}

func New(queueName string) (MQ, error) {
	var mq MQ = MQ{queueName: queueName}
	var err error

	mq.conn, err = nats.Connect(nats.DefaultURL)
	if err != nil {
		fmt.Printf("Failed to connect to NATS server: %s\n", err)
		return MQ{}, err
	}

	return mq, err
}

func (mq* MQ) BindHTTP(endpoint string, methods []string, callback SubCallbackType) {
	var natsEndpoint = "http" + strings.ReplaceAll(endpoint, "/", ".")

	for _, method := range methods {
		var subject string = natsEndpoint + "." + method
		fmt.Printf("SUB %s\n", subject)

		mq.conn.QueueSubscribe(subject, mq.queueName, func (m* nats.Msg) {
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
				fmt.Printf("json.Marshal failed: %s\n", err)
				return
			}
			m.Respond(data)
		})
	}
}

func NewResponse(req* HTTPRequest, status int, body* map[string]interface{}) *HTTPResponse {
	return &HTTPResponse{
		Type: req.Method, Fd: req.Fd, Status: status, Headers: map[string]string{
			"Content-Type": "application/json",
		},
		Body: body,
	}
}