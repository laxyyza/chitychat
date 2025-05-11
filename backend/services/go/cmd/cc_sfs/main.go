package main

import (
	"backend/services/go/internal/cc_sfs"
	"log"
)

func main() {
	sfs, err := cc_sfs.New()
	if err != nil {
		log.Panic(err)
	}

	err = sfs.Run()
	if err != nil {
		log.Panic(err)
	}
}
