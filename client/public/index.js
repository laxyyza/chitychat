
var session_id = localStorage.getItem("session_id");

console.log("Session ID:", session_id);

if (session_id)
    window.location.href = "/app";
else
    window.location.href = "/login";