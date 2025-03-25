package db

import (
	"context"
	"os"

	"github.com/jackc/pgx/v5"
)

type DB struct {
	conn* pgx.Conn
}

func New() (DB, error) {
	var db DB = DB{}
	var err error
	var url string = os.Getenv("DATABASE_URL")
	if url == "" {
		url = "dbname=chitychat"
	}

	db.conn, err = pgx.Connect(context.Background(), url)

	return db, err
}