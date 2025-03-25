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

func (s* Server) connect(address string) (net.Conn, error) {	
	conn, err := net.Dial("tcp", address)
	if (err != nil) {
		return nil, fmt.Errorf("failed to connect to server: %w", err)
	}
	log.Printf("Connected to server")
	return conn, nil
}

func (s* Server) Send(data map[string]interface{}) error {
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

func (s* Server) Recv() (map[string]interface{}, error) {
	buffer := make([]byte, 4096)

	n, err := s.conn.Read(buffer)
	if err != nil {
		return nil, fmt.Errorf("failed read: %w", err)
	}

	var ret map[string]interface{}

	err = json.Unmarshal(buffer[4:n], &ret)
	if err != nil {
		log.Fatalf("Error unmarshalling JSON: %w", err)
		return nil, err
	}

	return ret, nil
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