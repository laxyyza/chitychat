# Chity Chat
> Chity Chat is a simple Chat App with the purpose of learning. It includes a website code and a custom web server written in C.

### Chat App features
* **Multiple Groups:** Users can join any group; everything is public.
* **Real-time Communication:** Messages are sent and received instantly.
* **User Profile Pictures:** Users can upload and change their profile pictures.

### Planned features
- [ ] **Private Groups:** Invitation-only groups.
- [ ] **Group Member Roles:** Admins, mods, etc.
- [ ] **User Account Management:** Change username, display name, bio, and password.
- [ ] **Account Deletion:** Ability to delete accounts, messages, and groups.
- [ ] **Direct Messaging (DM):** Private messaging between users.
- [ ] **Enhanced Messaging:** Send photos, videos, files, reply to messages, and edit messages.
- [ ] **Mobile Compatibility:** Improve usability on mobile devices.
- [ ] **URL Parameters Support:** Support HTTP URL parameters.

### Web server features
* **HTTP/1.1 Parsing:** Basic parsing with support for GET and POST requests.
* **Web Sockets Implementation:** Real-time communication support.
* **SSL/TLS via OpenSSL:** Secure connections.
* **Password Security:** Salting and hashing using SHA512.
* **I/O Multiplexing:** Utilizes epoll for efficient I/O.
* **Database Management:** Uses SQLite3 for SQL database.
* **Session Management:** Users receive session IDs for persistent login.

### Learning experience
Through building Chity Chat, I learned:
* Web Server Implementation:
    * HTTP
    * Web Sockets
    * SSL/TLS
* SQL Database Usage with SQLite3.
* Frontend Development with JavaScript, HTML, and CSS.
* Password Security with SHA512 hashing.