package db

import (
	"context"
	"os"

	"github.com/jackc/pgx/v5/pgxpool"
)

type DB struct {
	Conn* pgxpool.Pool
}

func New() (DB, error) {
	var db DB = DB{}
	var err error
	var url string = os.Getenv("DATABASE_URL")
	if url == "" {
		url = "dbname=chitychat"
	}

	db.Conn, err = pgxpool.New(context.Background(), url)

	return db, err
}