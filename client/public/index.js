
let status_h1 = document.getElementById("status");

status_h1.innerHTML = "Connecting to server...";

let socket = new WebSocket("wss://" + window.location.host);

socket.addEventListener("open", (event) => {
    window.location.href = "/app";
});

socket.addEventListener("error", (event) => {
    window.location.href = "/login";
})

socket.addEventListener("close", (event) => {
    status_h1.innerHTML = "Lost connection to server.";
});
