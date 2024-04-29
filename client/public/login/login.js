let error_msg = document.getElementById("error_msg");

error_msg.innerHTML = "Not connected to server, yet";
let keep_logged_in = true;

function init()
{
    let socket = new WebSocket("wss://" + window.location.host);

    socket.addEventListener('open', (event) => {
        error_msg.innerHTML = "";
    });

    socket.addEventListener('message', (event) => {
        let respond = JSON.parse(event.data);

        console.log(event.data);

        if (respond.cmd === "session")
        {
            console.log("Sucsess: ", respond.id)
            localStorage.removeItem("session_id");
            localStorage.setItem("session_id", respond.id);
            window.location.href = "/app";
        }
        else if (respond.cmd === "error")
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

        let username = document.getElementById("login_username").value;
        let password = document.getElementById("login_password").value;

        let login_details = {
            cmd: "login",
            username: username,
            password: password,
            session: keep_logged_in
        };

        socket.send(JSON.stringify(login_details));
        error_msg.innerHTML = "";
    });

    document.getElementById("register_form").addEventListener("submit", (event) => {
        event.preventDefault();

        let username = document.getElementById("register_username").value;
        let displayname = document.getElementById("register_displayname").value;
        let password = document.getElementById("register_password").value;

        let login_details = {
            cmd: "register",
            username: username,
            displayname: displayname,
            password: password,
            session: keep_logged_in
        };

        socket.send(JSON.stringify(login_details));
        error_msg.innerHTML = "";
    });
}

init();
