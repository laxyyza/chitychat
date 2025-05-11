package cc_sfs

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"

	"github.com/google/uuid"
)

type Sfs struct {
	rootPath string
	fs http.Handler
	addr string
	mux* http.ServeMux
}

func setupLog() {
	exePath, _:= os.Executable()

	name := filepath.Base(exePath)
	log.SetPrefix(name + ": ")
	log.SetFlags(log.Ldate | log.Ltime | log.Lshortfile)
}

func (sfs* Sfs) uploadHandler(w http.ResponseWriter, r* http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Method not allowd", http.StatusMethodNotAllowed)
		return
	}

	err := r.ParseMultipartForm(10 << 20)
	if err != nil {
		log.Println(err)
		http.Error(w, "Could not parse form", http.StatusBadRequest)
		return
	}

	file, header, err := r.FormFile("file")
	if err != nil {
		log.Printf("FromFile: %v\n", err)
		http.Error(w, "Missing file", http.StatusBadRequest)
		return
	}
	defer file.Close()

	uuidDir := uuid.New().String()
	dirPath := filepath.Join(sfs.rootPath, uuidDir)

	err = os.MkdirAll(dirPath, 0755)
	if err != nil {
		log.Printf("os.MkDirAll(%s): %v\n", dirPath, err)
		http.Error(w, "Could not create directory", http.StatusInternalServerError)
		return
	}

	dstPath := filepath.Join(dirPath, header.Filename)
	dstFile, err := os.Create(dstPath)
	if err != nil {
		log.Printf("os.Create(%s): %v\n", dstPath, err)
		http.Error(w, "Unable to write file", http.StatusInternalServerError)
		return
	}
	defer dstFile.Close()

	_, err = io.Copy(dstFile, file)
	if err != nil {
		log.Printf("io.Copy: %s: %v\n", dstPath, err)
		http.Error(w, "Unable to save file", http.StatusInternalServerError)
		return
	}

	fmt.Printf("Uploaded file: %s/%s\n", uuidDir, header.Filename)

	json.NewEncoder(w).Encode(map[string]string{
		"endpoint": fmt.Sprintf("/f/%s/%s",  uuidDir, header.Filename),
	})
}

func loggingMiddleware(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        log.Printf("%s %s from %s", r.Method, r.URL.Path, r.RemoteAddr)
        next.ServeHTTP(w, r)
    })
}

func withCORS(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Allow all origins
		w.Header().Set("Access-Control-Allow-Origin", "*")

		// Optional headers
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")

		// Handle preflight request
		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusNoContent)
			return
		}

		// Pass to next handler
		next.ServeHTTP(w, r)
	})
}

func New() (*Sfs, error) {
	setupLog()
	rootPath, exists := os.LookupEnv("SFS_ROOT_PATH")
	if !exists {
		return nil, fmt.Errorf("missing SFS_ROOT_PATH")
	}
	addr, exists := os.LookupEnv("SFS_ADDR")
	if !exists {
		addr = ":8081"
	}

	sfs := Sfs{
		rootPath: rootPath,
		fs: http.FileServer(http.Dir(rootPath)),
		addr: addr,
		mux: http.NewServeMux(),
	}

	sfs.mux.Handle("/f/", loggingMiddleware(http.StripPrefix("/f/", sfs.fs)))
	sfs.mux.HandleFunc("/api/upload/", sfs.uploadHandler)

	return &sfs, nil
}

func (sfs* Sfs) Run() error {
	log.Printf("Listening on %s\n", sfs.addr)
	return http.ListenAndServeTLS(sfs.addr, "backend/server/server.crt", "backend/server/server.key", withCORS(sfs.mux))
}
