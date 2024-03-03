
var main_element = document.getElementById("main");
var input_msg = document.getElementById("msg_input");
var send_button = document.getElementById("msg_input_button");
var messages_container = document.getElementById("messages_con")
var group_member_container = document.getElementById("group_members_list_con");
var create_group_button = document.getElementById("create_group_button");
var join_group_button = document.getElementById("join_group_button");
var popup_container = document.getElementById("popup_con");
var popup = document.getElementById("popup");
var popup_create_group = document.getElementById("popup_create_group");
var popup_join_group = document.getElementById("popup_join_group");
var close_popup_button = document.getElementById("close_popup");
var group_class = document.getElementsByClassName("group");

var socket = new WebSocket("ws://" + window.location.host);

var input_pixels = 55;
const input_pixels_default = input_pixels;
const input_add_pixels = 30;
const input_max_height = input_pixels_default + (input_add_pixels * 10);
input_scroll_height = input_msg.scrollHeight;
input_client_height = input_msg.clientHeight;

var selected_group = null;

class User
{
    constructor(id, username, displayname, bio)
    {
        this.id = id;
        this.username = username;
        this.displayname = displayname;
        this.bio = bio;
    }
}

class Group
{
    constructor(id, name, group_members)
    {
        this.name = name;
        this.id = "group_" + id;
        console.log("new group: ", this.id, this.name);

        this.div_chat_messages = document.createElement("div");
        this.div_chat_messages.id = "messages";

        this.div_group_members = document.createElement("div");
        this.div_group_members.id = "group_members_list";

        this.div_list = document.createElement("div");
        this.div_list.className = "group";
        this.div_list.id = this.id;
        this.div_list.innerHTML = this.name;
        this.div_list.addEventListener("click", () => {
            if (selected_group)
            {
                selected_group.classList.remove("active");
            }

            this.div_list.classList.add("active");

            selected_group = this.div_list;

            document.getElementsByClassName("messages");

            var current_messages = document.getElementById("messages");
            var current_group_list = document.getElementById("group_members_list");

            if (current_messages)
                messages_container.replaceChild(this.div_chat_messages, current_messages);
            else
                messages_container.appendChild(this.div_chat_messages);

            if (current_group_list)
                group_member_container.replaceChild(this.div_group_members, current_group_list);
            else
                group_member_container.appendChild(this.div_group_members);

            messages_container.scrollTo(0, messages_container.scrollHeight);
        });

        if (group_members)
        {
            for (var i = 0; i < group_members.length; i++)
            {
                const member = group_members[i];
                this.add_member(member);
            }
        }

        group_list.appendChild(this.div_list);
    }

    add_member(member)
    {
        var div_member = document.createElement("div");
        div_member.className = "group_member";
        div_member.innerHTML = member.displayname;

        this.div_group_members.appendChild(div_member);
    }
}

var groups = [];
var client_user = null;

function start_popup_create_group(width, height)
{
    popup_container.style.display = "block";
    popup.style.width = width + "px";
    popup.style.height = height + "px";
    popup_create_group.style.display = "block";

    var create_button = document.getElementById("popup_create_group_button");
    var input = document.getElementById("popup_create_group_input");
    create_button.addEventListener("click", () => {
        if (input.value == "")
            return;

        var packet = {
            type: "group_create",
            name: input.value
        };

        socket.send(JSON.stringify(packet));

        // groups.push(new Group(-1, input.value, null));
        input.value = "";

        stop_popup();
    });
}

function start_popup_join_group(width, height)
{
    popup_container.style.display = "block";
    popup.style.width = width + "px";
    popup.style.height = height + "px";
    popup_join_group.style.display = "block";

    var groups = document.querySelectorAll(".popup_group");
    groups.forEach(group => {
        group.addEventListener("mouseenter", () => {
            var join_button = group.querySelector("#popup_join_group_button");
            join_button.style.backgroundColor = "rgb(50, 100, 50)";
            join_button.style.color = "white";

            join_button.addEventListener("mouseenter", () => {
                join_button.style.backgroundColor = "rgb(50, 200, 50)";
                join_button.style.color = "white";
            });

            join_button.addEventListener("mouseleave", () => {
                join_button.style.backgroundColor = "rgb(50, 100, 50)";
                join_button.style.color = "white";
            });
        });
        group.addEventListener("mouseleave", () => {
            var join_button = group.querySelector("#popup_join_group_button");
            join_button.style.backgroundColor = "inherit";
            join_button.style.color = "transparent";
        });
    });

}

function stop_popup()
{
    popup_container.style.display = "none";
    popup_create_group.style.display = "none";
    popup_join_group.style.display = "none";
}

close_popup_button.addEventListener("click", () => {
    stop_popup();
});

create_group_button.addEventListener("click", () => {
    start_popup_create_group(400, 200);
});

join_group_button.addEventListener("click", () => {
    start_popup_join_group(600, 800);
});

function set_input_height(lines)
{
    input_pixels = input_pixels_default + (input_add_pixels * lines);

    if (input_pixels > input_max_height)
        input_pixels = input_max_height;

    main_element.style.gridTemplateRows = "auto " + input_pixels + "px";
}

function increate_input_lines()
{
    input_pixels += input_add_pixels;

    if (input_pixels > input_max_height)
        input_pixels = input_max_height;

    main_element.style.gridTemplateRows = "auto " + input_pixels + "px";
}

function decrease_input_lines()
{
    input_pixels -= input_add_pixels;

    if (input_pixels > input_max_height)
        input_pixels = input_max_height;

    main_element.style.gridTemplateRows = "auto " + input_pixels + "px";
}

function create_msg_box(user, content)
{
    var div_msg = document.createElement("div");
    div_msg.className = "msg";

    var span_displayname = document.createElement("span");
    span_displayname.className = "msg_displayname";
    span_displayname.innerHTML = user.displayname;

    var span_timestamp = document.createElement("span");
    span_timestamp.className = "msg_timestamp";
    span_timestamp.innerHTML = "1AM Today";

    var div_content = document.createElement("div");
    div_content.className = "msg_content_con";

    var span_msg_content = document.createElement("span");
    span_msg_content.className = "msg_content";
    span_msg_content.innerHTML = content;
    div_content.appendChild(span_msg_content);

    div_msg.appendChild(span_displayname);
    div_msg.appendChild(span_timestamp);
    div_msg.appendChild(div_content);


    var messages = document.getElementById("messages");

    messages.appendChild(div_msg);

    messages_container.scrollTo(0, messages_container.scrollHeight);

    set_input_height(0);
}

function send_message()
{
    const msg = input_msg.value;
    if (msg == "")
        return;

    user = {
        displayname: client_user.displayname 
    };

    create_msg_box(user, msg);
    
    console.log("Sending msg:", msg);

    input_msg.value = "";
}

send_button.addEventListener('click', () => {
    send_message();
});

input_msg.addEventListener('keydown', (event) => {
    if (event.key === "Enter")
    {
        console.log("ENTER");
        if (!event.shiftKey)
        {
            event.preventDefault();
            send_message();
        }
    }
});

var old_new_line_count = 0;

input_msg.addEventListener('input', () => {
    // const value = input_msg.value;
    // const new_line_count = (value.match(/\n/g) || []).length;
    // if (old_new_line_count != new_line_count)
    // {
    //     set_input_height(new_line_count);
    //     old_new_line_count = new_line_count;
    // }

    if (input_msg.scrollHeight > input_msg.clientHeight)
    {
        increate_input_lines();
    }
    else if (input_msg.scrollHeight < input_msg.clientHeight)
    {
        decrease_input_lines();
    }
});

var logged_in = false;

socket.addEventListener("open", (event) => {
    var session_id = localStorage.getItem("session_id");
    if (!session_id)
    {
        window.location.href = "/login";
        return;
    }

    var session_packet = {
        type: "session",
        id: session_id
    };

    socket.send(JSON.stringify(session_packet));
});

socket.addEventListener("message", (event) => {
    const packet = JSON.parse(event.data);

    console.log(packet);

    if (logged_in)
    {
        if (packet.type === "client_user_info")
        {
            client_user = new User(packet.id, packet.username, packet.displayname, packet.bio);
            var me_displayname = document.getElementById("me_displayname");
            me_displayname.innerHTML = client_user.displayname;
            var me_username = document.getElementById("me_username");
            me_username.innerHTML = client_user.username;
        }
        else if (packet.type === "group")
        {
            groups.push(new Group(packet.id, packet.name, null));
        }
        else if (packet.type === "client_groups")
        {
            for (var i = 0; i < packet.groups.length; i++)
            {
                const group = packet.groups[i];
                groups.push(new Group(group.id, group.name, null));
            }
        }
        else
        {
            console.warn("Packet type: " + packet.type + " not implemented");
        }
    }
    else
    {
        if (packet.type == "session")
        {
            logged_in = true;

            const req_user_info = {
                type: "client_user_info"
            };
            const req_groups = {
                type: "client_groups"
            };

            socket.send(JSON.stringify(req_user_info));

            socket.send(JSON.stringify(req_groups));
        }
        else
        {
            window.location.href = "/login";
        }
    }
});

socket.addEventListener("error", (event) => {

});

socket.addEventListener("close", (event) => {

});