let error_msg = document.getElementById("error_msg");

error_msg.innerHTML = "Not connected to server, yet";

function init()
{
    let socket = new WebSocket("wss://" + window.location.host);

    socket.addEventListener('open', (event) => {
        error_msg.innerHTML = "";
    });

    socket.addEventListener('message', (event) => {
        var respond = JSON.parse(event.data);

        console.log(event.data);

        if (respond.type === "session")
        {
            console.log("Sucsess: ", respond.id)
            localStorage.removeItem("session_id");
            localStorage.setItem("session_id", respond.id);
            window.location.href = "/app";
        }
        else if (respond.type === "error")
        {
            error_msg.innerHTML = respond.error_msg;
        }
    });

    socket.addEventListener('error', (event) => {
        error_msg.innerHTML = "Connection error: " + event.data;
    });

    socket.addEventListener('close', (event) => {
        error_msg.innerHTML = "Lost connection";
        init();
    });

    document.getElementById("login_form").addEventListener("submit", (event) => {
        event.preventDefault();

        var username = document.getElementById("login_username").value;
        var password = document.getElementById("login_password").value;

        var login_details = {
            type: "login",
            username: username,
            password: password
        };

        socket.send(JSON.stringify(login_details));
        error_msg.innerHTML = "";
    });

    document.getElementById("register_form").addEventListener("submit", (event) => {
        event.preventDefault();

        var username = document.getElementById("register_username").value;
        var displayname = document.getElementById("register_displayname").value;
        var password = document.getElementById("register_password").value;

        var login_details = {
            type: "register",
            username: username,
            displayname: displayname,
            password: password
        };

        socket.send(JSON.stringify(login_details));
        error_msg.innerHTML = "";
    });
}

init();