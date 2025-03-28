package server

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"net"
)

type Server struct {
	conn net.Conn
	address string
}

type HTTPRequest struct {
	Type 	string `json:"type"`
	Fd 		int `json:"fd"`
	UserID 	uint32 `json:"user_id"`
	Path 	string `json:"path"`
	Headers map[string]string `json:"headers"`
	Params  map[string]string `json:"params"`
	Body	map[string]interface{} `json:"body"`
}

type HTTPResponse struct {
	Type 	string `json:"type"`
	Fd 		int `json:"fd"`
	Status 	int `json:"status"`
	Headers map[string]string `json:"headers"`
	Body*	map[string]interface{} `json:"body"`
}

func (s* Server) connect(address string) (net.Conn, error) {	
	conn, err := net.Dial("tcp", address)
	if (err != nil) {
		return nil, fmt.Errorf("failed to connect to server: %w", err)
	}
	log.Printf("Connected to server")
	return conn, nil
}

func (s* Server) Send(data interface{}) error {
	jsonBytes, err := json.Marshal(data)
	if err != nil {
		fmt.Printf("Invalid data: %w\n", err)
		return err
	}

	var buf bytes.Buffer
	binary.Write(&buf, binary.NativeEndian, uint32(len(jsonBytes)))
	buf.Write(jsonBytes)

	message := buf.Bytes()

	_, err = s.conn.Write(message)

	return err
}

func (s* Server) SendResp(data* HTTPResponse) error {
	return s.Send(data)
}

func (s* Server) Recv() (*HTTPRequest, error) {
	buffer := make([]byte, 4096)

	n, err := s.conn.Read(buffer)
	if err != nil {
		return nil, fmt.Errorf("failed read: %w", err)
	}

	var data HTTPRequest

	err = json.Unmarshal(buffer[4:n], &data)
	if err != nil {
		log.Fatalf("Error unmarshalling JSON: %w", err)
		return nil, err
	}

	return &data, nil
}

func New() (Server, error) {
	var err error
	var address string = "localhost:6000"
	var server Server = Server{address: address}

	server.conn, err = server.connect(address)

	return server, err
}

func (s* Server) Close() {
	s.conn.Close()
}

func NewResponse(req* HTTPRequest, status int, body* map[string]interface{}) *HTTPResponse {
	return &HTTPResponse{Type: req.Type, Fd: req.Fd, Status: status, Headers: map[string]string{
		"Content-Type": "application/json",
	},
	Body: body,
}
}