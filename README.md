# Chity Chat
> A simple Chat App

### What is in here?
* Website code, and custom web server written in C.

### Why do this?
* Learning purposes.

### Chat App features:
> NOTE: It's simple
* Multiple groups, everything is public; anyone can join any group.
* Real-time communcation with server, sending/recieving messages instantaly.
* User profile picutres and can change it.

### Features planned:
- [ ] Private groups, can only join from invites via links/codes.
- [ ] Group member roles (e.g. admin, mods, etc in a group).
    - [ ] Kick/ban users from group 
- [ ] Change username, display name, bio, and passwords.
- [ ] Delete account, messages and groups.
- [ ] Leave groups.
- [ ] Direct messaging (DM).
- [ ] See more information about other users.
- [ ] Messages:
    - [ ] Sending photos, videos and files.
    - [ ] Reply to a message.
    - [ ] Edit your message.
- [ ] Mobile friendly (It's currently only for desktop use).
- [ ] (Web Server) Support HTTP URL parameters.

### Web server features:
* Basic HTTP/1.1 parsing, only supports GET and POST requests.
* Web Sockets implementation (can parse the web socket frame, unmask it and sending it).
* SSL/TLS (https, wss) via OpenSSL.
* Password salting and hashing using SHA512. 
* epoll for I/O multiplexing.
* Using SQLite3 for SQL database engine.
* Doubly linked-list for clients.
* Basic session management (i.e. After client login, client will recieve session ID and can be used later so you dont have to retype username passwords).

### What I learnent by doing this:
> I am already fimialiar doing network programming in C for Linux/Posix. 
* How to make a web server, implementing:
    * HTTP (basic, only supports GET and POST requests)
    * Web Sockets
    * SSL/TLS
* SQL (SQLite3) (my first project using a SQL database)
* How to make a web site (JavaScript, HTML and CSS)
* Securly save passwords in database, using SHA512.
