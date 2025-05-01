package db

import (
	"context"
	"fmt"
	"os"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgconn"
	"github.com/jackc/pgx/v5/pgxpool"
)

type DB struct {
	Conn* pgxpool.Pool

	SQL map[string]string
}

func New() (DB, error) {
	var db DB = DB{SQL: map[string]string{}}
	var err error
	var url string = os.Getenv("DATABASE_URL")
	if url == "" {
		url = "dbname=chitychat"
	}

	db.Conn, err = pgxpool.New(context.Background(), url)

	return db, err
}

func (db* DB) LoadSQLFiles(names []string) error {
	for _, name := range names {
		path := fmt.Sprintf("backend/sql/%s.sql", name)

		data, err := os.ReadFile(path)
		if err != nil {
			fmt.Printf("Failed to read: %s: %v\n", path, err)
			return err
		}

		db.SQL[name] = string(data)
	}
	return nil
}

func (db* DB) Query(query_or_name string, args ...any) (pgx.Rows, error) {
	var query string
	query, ok := db.SQL[query_or_name]
	if !ok {
		query = query_or_name
	}

	return db.Conn.Query(context.Background(), query, args...) 
}

func (db* DB) QueryRow(query_or_name string, args ...any) pgx.Row {
	var query string
	query, ok := db.SQL[query_or_name]
	if !ok {
		query = query_or_name
	}

	return db.Conn.QueryRow(context.Background(), query, args...) 
}

func (db* DB) Exec(query_or_name string, args ...any) (pgconn.CommandTag, error) {
	var query string
	query, ok := db.SQL[query_or_name]
	if !ok {
		query = query_or_name
	}

	return db.Conn.Exec(context.Background(), query, args...) 
}
