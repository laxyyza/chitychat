
let session_id = localStorage.getItem("session_id");
let status_h1 = document.getElementById("status");

console.log("Session ID:", session_id);

if (session_id)
{
    let socket = new WebSocket("wss://" + window.location.host);
    status_h1.innerHTML = "Connecting to server...";

    socket.addEventListener("open", (event) => {
        status_h1.innerHTML = "Connected to server. Waiting for server";
        var packet = {
            type: "session",
            id: session_id
        };

        socket.send(JSON.stringify(packet));
    });

    socket.addEventListener("message", (event) => {
        const packet = JSON.parse(event.data);
        if (packet.type == "session")
        {
            status_h1.innerHTML = "Good, switching to /app";
            window.location.href = "/app";
        }
        else
        {
            console.log(packet);
            if (packet.type === "error")
                status_h1.innerHTML = "Error: " + packet.error_msg;
            localStorage.removeItem("session_id");
            window.location.href = "/login";
        }
    })

    socket.addEventListener("error", (event) => {
        console.log("error:", event.data);
        status_h1.innerHTML = "Socket error: " + event.data;
    })

    socket.addEventListener("close", (event) => {
        status_h1.innerHTML = "Lost connection to server.";
    });

    // window.location.href = "/app";
}
else
    window.location.href = "/login";
