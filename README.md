### Table of Contents
1. [Chity Chat](#chity-chat)
2. [Chat App Features](#chat-app-features)
3. [Web Server Features](#web-server-features)
4. [Web Server Limitations](#web-server-limitations)
5. [Current Web Server Challenges](#current-web-server-challenges)
6. [Planned Features](#planned-features)
7. [Learning Experience](#learning-experience)
8. [Project Directory Structure](#project-directory-structure)
9. [Build Web Server](#build-web-server)
10. [Coding Style](#coding-style)

# Chity Chat
### This is a simple Chat App with the purpose of learning.
* Fullstack Project
   * Frontend: In JavaScript, HTML and CSS. (My first time)
   * Backend: Custom web server written in C.
> Though the front-end is my initial venture into web development, the standout feature lies in the C-written backend.

## Chat App Features
* **Private/Public Groups:** Users have the option to join public groups or enter private ones using specific codes.
* **Real-time Communication:** Messages are sent and received instantly.
* **Real-time User Staus Updates:** Instantly see if users comes online and offline.

## Web Server Features
* **HTTP/1.1 Parsing:** Basic parsing with support for GET and POST requests.
* **Web Sockets Implementation:** Real-time communication support.
* **SSL/TLS via OpenSSL:** Secure connections.
* **Password Security:** Salting and hashing using SHA512.
* **I/O Multiplexing:** Utilizes [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html) for efficient I/O.
* **Database Management:** Uses PostgreSQL for SQL database.
* **Session Management:** Users receive session IDs for persistent login.
* **Multi-Threaded:** Utilizes a thread pool to efficiently manage concurrent events.
* **IP Versions:** Supports both IPv4 and IPv6.

## Web Server limitations
* Exclusively designed for this Chat App.
* **Basic HTTP Parsing:** Limited to handling only GET and POST requests.
* **Monolithic Architecture:** Combines web server and chat server functionalities within a single process.

## Current Web Server Challenges
* ~~Experiences significant slowdowns with only approximately 100 concurrent users.~~ Fixed.
* ~~Encounters unexpected errors when multiple clients disconnect simultaneously.~~ Fixed.
* ~~Worker threads are heavly I/O bound to PostgreSQL. Non-blocking postgres connection maybe?~~ 
* Faces potential deadlock issues during high load, resulting in server unresponsiveness.

## Planned Features
- [x] **Private Groups:** Invitation-only groups.
   - [X] Mark group as private, only group members can get it.
   - [X] Implement Invite Codes
   - [ ] Implement Invite Links
- [ ] **Group Member Roles:** Admins, mods, etc.
- [ ] **User Account Management:** Change username, display name, bio, and password.
- [ ] **Deletion:** Ability to delete accounts, messages, and groups.
   - [x] Delete messages.
   - [ ] Delete accounts.
   - [x] Detete groups. 
- [x] **Direct Messaging (DM):** Private messaging between users (Create Private Group with only 2 users).
- [ ] **Enhanced Messaging:** Send photos, videos, files, reply to messages, and edit messages.
   - [X] Front-end implementation (images)
      - [X] Add images and paste image from clipboard.
   - [X] Back-end implementation (images) 
- [ ] **Mobile Compatibility:** Improve usability on mobile devices.
- [X] **URL Parameters Support:** Support HTTP URL parameters.
- [ ] **Real-Time User Status Management:** Online, Offline, Away, busy, typing, etc.
   - [x] User Online/Offline
   - [ ] Typing.
   - [ ] User set status
- [x] **Upload File Management:** Avoid duplication user files.
   - [x] Default profile pic
- [ ] **Real-Time User Profile Updates:** Receive instant updates for user profile changes like usernames, display names, bio, and profile pictures.
   - [x] Profile pictures.
   - [ ] Username / Displayname
   - [ ] Bio 

## Learning experience
> Through building Chity Chat, I learned:
* Web Server Implementation:
    * HTTP
    * Web Sockets
    * SSL/TLS
    * Multi-Threading
* SQL Database Usage with SQLite3, later PostgreSQL.
* Frontend Development with JavaScript, HTML, and CSS.
* Password Security with SHA512 hashing.

## Project Directory Structure
```
# chitychat's root
/
├── server/        # Server/Backend 
│   ├── src/       # Server C source code
│   ├── include/   # Server Header Files
│   └── sql/       # SQL schema and queries
│
├── client/        # Client/Frontend
│   ├── public/    # Default Public Directory & Website source code
│   └── headless/  # Headless client; bot source code
│
└── tests/         # Currently only have load_balance_bots.sh
```

## Build Web Server
> [!NOTE]
> Exclusively for Linux.
### Dependencies
* clang
* json-c
* libmagic
* postgresql
* openssl
* meson
> Dependencies Package Names
>* Arch: `json-c file postgresql openssl meson clang`
>* Debian: `libjson-c-dev libmagic-dev postgresql postgresql-client libpq-dev libssl-dev meson clang` 

### Clone repo and cd into:
```
git clone https://github.com/laxyyza/chitychat.git && cd chitychat
```
### Setup build directory:
```
CC=clang meson setup build/
```
### Compile:
```
ninja -C build/
```
Executable name: `chitychat` inside `build/`
## After compilation: PostgreSQL and SSL certificates.
### PostgreSQL:
Read the [Arch Wiki](https://wiki.archlinux.org/title/PostgreSQL) (or your distro wiki) for setting up PostgreSQL.
> After you setup PostgreSQL, create a PostgreSQL database called: `chitychat`
```
sudo -u postgres createdb chitychat
```
### SSL certificates:
Generate self-signed SSL keys: (in project's root directory)
> [!NOTE]
> Your browser will display a warning when using self-signed SSL keys.
```
openssl req -x509 -newkey rsa:4096 -keyout server/server.key -out server/server.crt -days 365 -nodes
```
Now you _should_ be done, and execute `build/chitychat` server.

Config file: `server/config.json`

### In your browser: `https://localhost:8080` or `https://` + ip address + `:8080`

Change port in config file or use `--port` arguments. See `build/chitychat --help`

## Coding Style
* https://github.com/laxyyza/stdcode
