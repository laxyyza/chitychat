
socket = new WebSocket("ws://" + window.location.host);

socket.addEventListener('open', (event) => {

});

socket.addEventListener('message', (event) => {
    var respond = JSON.parse(event.data);

    console.log(event.data);

    if (respond.type === "session")
    {
        console.log("Sucsess: ", respond.id)
        localStorage.setItem("session_id", respond.id);
    }
    else
    {
    }
});

socket.addEventListener('error', (event) => {
    
});

socket.addEventListener('close', (event) => {
    
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
});