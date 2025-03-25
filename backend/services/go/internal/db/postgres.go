package db

import (
	"context"
	"fmt"
	"os"

	"github.com/jackc/pgx/v5"
)

func Connect() (*pgx.Conn, error) {
	conn, err := pgx.Connect(context.Background(), os.Getenv("DATABASE_URL"))	
	if err != nil {
		fmt.Printf("Unable to connect to databse: %v\n", err)
		return nil, err
	}

	return conn, nil
}