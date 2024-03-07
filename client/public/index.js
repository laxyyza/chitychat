
var session_id = localStorage.getItem("session_id");

console.log("Session ID:", session_id);

if (session_id)
{
    var socket = new WebSocket("wss://" + window.location.host);

    socket.addEventListener("open", (event) => {
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
            window.location.href = "/app";
        }
        else
        {
            console.log(packet);
            localStorage.removeItem("session_id");
            window.location.href = "/login";
        }
    })

    socket.addEventListener("error", (event) => {
        console.log("error:", event.data);
    })

    socket.addEventListener("close", (event) => {

    });

    // window.location.href = "/app";
}
else
    window.location.href = "/login";
