# Chity Chat
### This is a simple Chat App with the purpose of learning.
* Fullstack Project
   * Frontend: In JavaScript, HTML and CSS. (My first time)
   * Backend: Custom web server written in C.
> Though the front-end is my initial venture into web development, the standout feature lies in the C-written backend.

## Chat App features
* **Private/Public Groups:** Users have the option to join public groups or enter private ones using specific codes.
* **Real-time Communication:** Messages are sent and received instantly.
* **Real-time User Staus Updates:** Instantly see if users comes online and offline.

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

## Web Server Features
* **HTTP/1.1 Parsing:** Basic parsing with support for GET and POST requests.
* **Web Sockets Implementation:** Real-time communication support.
* **SSL/TLS via OpenSSL:** Secure connections.
* **Password Security:** Salting and hashing using SHA512.
* **I/O Multiplexing:** Utilizes [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html) for efficient I/O.
* **Database Management:** Uses PostgreSQL for SQL database.
* **Session Management:** Users receive session IDs for persistent login.
* **Multi-Threaded:** Utilizes a thread pool to efficiently manage concurrent events.

## Web Server limitations
* **Basic HTTP Parsing:** Limited to handling only GET and POST requests.
* **Monolithic Architecture:** Combines web server and chat server functionalities within a single process, requiring recompilation for any server code modifications.

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
git clone https://github.com/laxyy69/chitychat.git && cd chitychat
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
```
openssl req -x509 -newkey rsa:4096 -keyout server/server.key -out server/server.crt -days 365 -nodes
```
Now you _should_ be done, and execute `build/chitychat` server.

Config file: `server/config.json`

### In your browser: `https://localhost:8080` or `https://` + ip address + `:8080`

Change port in config file or use `--port` arguments. See `build/chitychat --help`

## Coding Style
* https://github.com/laxyy69/stdcode
