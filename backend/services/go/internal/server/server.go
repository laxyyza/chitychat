package server

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"log"
	"net"
)

func Connect(address string) (net.Conn, error) {	
	conn, err := net.Dial("tcp", address)
	if (err != nil) {
		return nil, fmt.Errorf("failed to connect to server: %w", err)
	}
	log.Printf("Connected to server")
	return conn, nil
}

func Send(conn net.Conn, data map[string]interface{}) error {
	jsonBytes, err := json.Marshal(data)
	if err != nil {
		fmt.Printf("Invalid data: %w\n", err)
		return err
	}

	var buf bytes.Buffer
	binary.Write(&buf, binary.NativeEndian, uint32(len(jsonBytes)))
	buf.Write(jsonBytes)

	message := buf.Bytes()

	_, err = conn.Write(message)

	return err
}

func Recv(conn net.Conn) (map[string]interface{}, error) {
	buffer := make([]byte, 4096)

	n, err := conn.Read(buffer)
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