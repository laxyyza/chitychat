# Chity Chat
>### Chity Chat is a simple Chat App with the purpose of learning.
* Fullstack Project
   * Frontend: In JavaScript/html/css
   * Backend: Custom web server written in C.

## Chat App features
* **Multiple Groups:** Users can join any group; everything is public.
* **Real-time Communication:** Messages are sent and received instantly.
* **User Profile Pictures:** Users can upload and change their profile pictures.

## Planned Features
- [x] **Private Groups:** Invitation-only groups.
   - [X] Mark group as private, only group members can get it.
   - [X] Implement Invite Codes
   - [ ] Implement Invite Links  
      - [ ] Link management (e.g. https://localhost:8080/invites/6141611) - how would I implement this? 
- [ ] **Group Member Roles:** Admins, mods, etc.
- [ ] **User Account Management:** Change username, display name, bio, and password.
- [ ] **Deletion:** Ability to delete accounts, messages, and groups.
   - [x] Delete messages.
   - [ ] Delete accounts.
   - [ ] Detete groups. 
- [ ] **Direct Messaging (DM):** Private messaging between users.
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

## Coding Style
* https://github.com/laxyy69/stdcode
