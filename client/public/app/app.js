import {Group} from './group.js';
import {User} from './user.js';
import {Server} from './server.js';
// import {socket} from './ws.js';

export class App
{
    constructor()
    {
        this.groups = {};
        this.users = {};
        this.client_user = null;
        this.current_group = null;
        this.selected_group = null;

        this.main_element = document.getElementById("main");
        this.input_msg = document.getElementById("msg_input");
        this.send_button = document.getElementById("msg_input_button");
        this.messages_container = document.getElementById("messages_con");
        this.group_member_container = document.getElementById("group_members_list_con");
        this.create_group_button = document.getElementById("create_group_button");
        this.join_group_button = document.getElementById("join_group_button");
        this.popup_container = document.getElementById("popup_con");
        this.popup = document.getElementById("popup");
        this.popup_create_group = document.getElementById("popup_create_group");
        this.popup_join_group = document.getElementById("popup_join_group");
        this.popup_settings = document.getElementById("popup_settings");
        this.popup_settings.style.display = "none";
        this.settings_button = document.getElementById("settings_button");
        this.close_popup_button = document.getElementById("close_popup");
        this.group_class = document.getElementsByClassName("group");
        this.edit_account_apply = document.getElementById("edit_account_apply");
        this.img_ele = document.getElementById("settings_img");

        this.groups = {};
        this.current_group;
        this.selected_group = null;
        this.users = {};
        this.client_user = null;

        this.input_pixels = 55;
        this.input_pixels_default = this.input_pixels;
        this.input_add_pixels = 30;
        this.input_max_height = this.input_pixels_default + (this.input_add_pixels * 10);

        this.popup_join = false;

        this.logged_in = false;

        this.close_popup_button.addEventListener("click", () => {
            this.stop_popup();
        });

        this.create_group_button.addEventListener("click", () => {
            this.start_popup_create_group(400, 200);
        });

        this.join_group_button.addEventListener("click", () => {
            this.start_popup_join_group(600, 800);
        });

        this.send_button.addEventListener('click', () => {
            this.send_message();
        });

        this.settings_button.addEventListener('click', () => {
            this.start_popup_settings();
        });

        this.edit_account_apply.addEventListener('click', () => {
            const upload_file = document.getElementById("upload_pfp");
            this.new_pfp_file = upload_file.files[0];
            let new_pfp = false;
            if (this.new_pfp_file)
                new_pfp = true;

            const edit_account_packet = {
                type: "edit_account",
                new_username: null,
                new_displayname: null,
                new_pfp: new_pfp
            };

            this.server.ws_send(edit_account_packet);
        });

        this.input_msg.addEventListener('keydown', (event) => {
            if (event.key === "Enter")
            {
                console.log("ENTER");
                if (!event.shiftKey)
                {
                    event.preventDefault();
                    this.send_message();
                }
            }
        });

        this.input_msg.addEventListener('input', () => {
            // const value = input_msg.value;
            // const new_line_count = (value.match(/\n/g) || []).length;
            // if (old_new_line_count != new_line_count)
            // {
            //     set_input_height(new_line_count);
            //     old_new_line_count = new_line_count;
            // }

            if (this.input_msg.scrollHeight > this.input_msg.clientHeight)
            {
                this.increate_input_lines();
            }
            else if (this.input_msg.scrollHeight < this.input_msg.clientHeight)
            {
                this.decrease_input_lines();
            }
        });

        this.server = new Server(this);
    }

    start_popup_settings()
    {
        this.popup_settings.style.display = "block";
        this.popup_container.style.display = "block";
        this.popup.style.width = "70%";
        this.popup.style.height = "70%";
    }

    server_open(event)
    {
        const session_id = localStorage.getItem("session_id");
        if (!session_id)
        {
            window.location.href = "/login";
            return;
        }

        const session_packet = {
            type: "session",
            id: session_id
        };

        this.server.ws_send(session_packet);
    }

    server_error()
    {

    }

    server_msg(packet)
    {
        console.log(packet);

        if (this.logged_in)
        {
            if (packet.type === "client_user_info")
            {
                this.client_user = new User(packet.id, packet.username, packet.displayname, packet.bio, packet.created_at, packet.pfp_name);
                this.users[this.client_user.id] = this.client_user;
                // users.push(client_user);
                let me_displayname = document.getElementById("me_displayname");
                me_displayname.innerHTML = this.client_user.displayname;
                let me_username = document.getElementById("me_username");
                me_username.innerHTML = this.client_user.username;
                this.img_ele.src = this.client_user.pfp_url;
            }
            else if (packet.type === "client_groups")
            {
                for (let i = 0; i < packet.groups.length; i++)
                {
                    const group = packet.groups[i];
                    const new_group = new Group(group.id, group.name, group.desc, group.members_id);
                    this.groups[group.id] = new_group;

                    if (this.popup_join)
                    {
                        let joining_button = document.querySelector("[joining_group=\"" + new_group.id + "\"]");
                        if (joining_button)
                        {
                            joining_button.innerHTML = "Joined";
                            joining_button.classList.remove("joining");
                            joining_button.classList.add("joined");
                        }
                    }
                }
            }
            else if (packet.type === "group_members")
            {
                let group = this.groups[packet.group_id];
                if (!group)
                {
                    console.warn("no group", packet.group_id);
                    return;
                }
                
                for (let i = 0; i < packet.members.length; i++)
                {
                    const user_id = packet.members[i];
                    group.members_id.push(user_id);
                    const user = this.users[user_id];
                    // const user = check_have_user(user_id);
                    if (user)
                    {
                        group.add_member(user);
                        continue;
                    }

                    const new_packet = {
                        type: "get_user",
                        id: user_id
                    };

                    this.server.ws_send(new_packet);
                }
            }
            else if (packet.type === "get_user")
            {
                if (this.users[packet.id])
                    return;
                // if (get_user(packet.id))
                //     return;
                let new_user = new User(packet.id, packet.username, packet.displayname, packet.bio, packet.created_at, packet.pfp_name);
                this.users[new_user.id] = new_user;

                for (let group_id in this.groups)
                {
                    let group = this.groups[group_id];
                    for (let m = 0; m < group.members_id.length; m++)
                    {
                        let member_id = group.members_id[m];
                        if (member_id === new_user.id)
                        {
                            group.add_member(new_user);

                            let user_msgs = group.div_chat_messages.querySelectorAll("[msg_user_id=\"" + new_user.id + "\"]");
                            for (let i = 0; i < user_msgs.length; i++)
                            {
                                const msg = user_msgs[i];
                                let msg_displayname = msg.querySelector(".msg_displayname");
                                msg_displayname.innerHTML = new_user.displayname;
                                let msg_img = msg.querySelector(".msg_img");
                                msg_img.src = new_user.pfp_url;
                            }

                            break;
                        }
                    }
                }

                if (this.popup_join)
                {
                    let owner_lists = document.querySelectorAll("#popup_group_owner" + new_user.id);
                    let owner_username_lists = document.querySelectorAll("#popup_group_owner_username" + new_user.id);

                    for (let i = 0; i < owner_lists.length; i++)
                    {
                        // let span_owner = document.getElementById("popup_group_owner" + new_user.id);
                        let span_owner = owner_lists[i];
                        span_owner.innerHTML = new_user.displayname;
                    }

                    for (let i = 0; i < owner_username_lists.length; i++)
                    {
                        // let span_owner_username = document.getElementById("popup_group_owner_username" + new_user.id);
                        let span_owner_username = owner_username_lists[i];
                        span_owner_username.innerHTML = new_user.username;
                    }
                }
            }
            else if (packet.type == "get_all_groups")
            {
                let requested_users = [];
                for (let i = 0; i < packet.groups.length; i++)
                {
                    let group = packet.groups[i];
                    let owner = this.users[group.owner_id];
                    // let owner = get_user(group.owner_id);
                    if (owner)
                    {
                        group.owner = {
                            username: owner.username,
                            displayname: owner.displayname 
                        };
                    }
                    else
                    {
                        group.owner = {
                            username: "?",
                            displayname: "?"
                        };

                        if (!requested_users.includes(group.owner_id))
                        {
                            const req_packet = {
                                type: "get_user",
                                id: group.owner_id
                            };

                            // const json_string = JSON.stringify(req_packet);
                            this.server.ws_send(req_packet);

                            requested_users.push(group.owner_id);
                        }
                    }

                    this.popup_join_group_add_group(group);
                }

                this.start_popup_join_group_style_modifiers();
            }
            else if (packet.type === "group_msg")
            {
                let user = this.users[packet.user_id];
                let group = this.groups[packet.group_id];
                if (!user)
                {
                    user = {
                        displayname: toString(packet.user_id),
                    };
                }

                group.add_msg(user, packet);
            }
            else if (packet.type === "get_group_msgs")
            {
                let group = this.groups[packet.group_id];

                for (let i = 0; i < packet.messages.length; i++)
                {
                    const msg = packet.messages[i];
                    let user = this.users[msg.user_id];
                    if (!user)
                    {
                        user = {
                            id: msg.user_id,
                            displayname: msg.user_id
                        };

                        const get_user_packet = {
                            type: "get_user",
                            id: msg.user_id
                        };
                        this.server.ws_send(get_user_packet);
                    }
                    group.add_msg(user, msg);
                }
            }
            else if (packet.type === "join_group")
            {
                let group = this.groups[packet.group_id];
                let user = this.users[packet.user_id];
                group.members_id.push(packet.user_id);
                if (!user)
                {
                    const get_user_packet = {
                        type: "get_user",
                        id: packet.user_id
                    };

                    this.server.ws_send(get_user_packet);
                    return;
                }

                group.add_member(user);
            }
            else if (packet.type === "edit_account")
            {
                const upload_token = packet.upload_token;
                const file = this.new_pfp_file;

                console.log(file);

                const reader = new FileReader();
                reader.onload = (event) => {
                    const data = event.target.result;

                    console.log("data", data);

                    const headers = new Headers();
                    headers.set("Upload-Token", upload_token);

                    fetch("/img/" + file.name, {
                        method: 'POST',
                        headers: {
                            "Upload-Token": upload_token
                        },
                        body: data
                    })
                    .then(response => {
                        if (!response.ok)
                            throw new Error("Failed");

                        return response.blob();
                    })
                    .then(blob => {
                        console.log("BLOB", blob);
                        const image_url = URL.createObjectURL(blob);
                        const img_ele = document.getElementById("settings_img");
                        img_ele.src = image_url;

                        URL.revokeObjectURL(image_url);
                    })
                    .catch(error => {
                        console.error("Failed to upload", error)
                    });
                };

                reader.readAsArrayBuffer(file);
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
                this.logged_in = true;

                const req_user_info = {
                    type: "client_user_info"
                };
                const req_groups = {
                    type: "client_groups"
                };

                this.server.ws_send(req_user_info);
                this.server.ws_send(req_groups);
            }
            else
            {
                window.location.href = "/login";
            }
        }
    }

    server_close(event)
    {
        this.logged_in = false;
    }

    start_popup_create_group(width, height)
    {
        this.popup_container.style.display = "block";
        this.popup.style.width = width + "px";
        this.popup.style.height = height + "px";
        this.popup_create_group.style.display = "block";

        let create_button = document.getElementById("popup_create_group_button");
        let input = document.getElementById("popup_create_group_input");
        create_button.addEventListener("click", () => {
            if (input.value == "")
                return;

            const packet = {
                type: "group_create",
                name: input.value
            };

            this.server.ws_send(packet);

            // groups.push(new Group(-1, input.value, null));
            input.value = "";

            this.stop_popup();
        });
    }

    start_popup_join_group(width, height)
    {
        this.popup_container.style.display = "block";
        this.popup.style.width = width + "px";
        this.popup.style.height = height + "px";
        this.popup_join_group.style.display = "block";
        let group_list = document.getElementById("popup_group_list_con");
        group_list.innerHTML = "";

        const packet = {
            type: "get_all_groups"
        };

        this.server.ws_send(packet);
        this.popup_join = true;
    }

    popup_join_group_add_group(group)
    {
        let group_div = document.createElement("div");
        group_div.className = "popup_group";

        let span_name = document.createElement("span");
        // span_name.id = "popup_group_name";
        span_name.className = "popup_group_name";
        span_name.innerHTML = group.name;
        group_div.appendChild(span_name);

        let span_owner = document.createElement("span");
        span_owner.className = "popup_group_owner";
        span_owner.id = "popup_group_owner" + group.owner_id;
        span_owner.innerHTML = group.owner.displayname;
        group_div.appendChild(span_owner);

        let span_owner_username = document.createElement("span");
        span_owner_username.className = "popup_group_owner_username";
        span_owner_username.id = "popup_group_owner_username" + group.owner_id;
        span_owner_username.innerHTML = group.owner.username;
        group_div.appendChild(span_owner_username);

        let join_button = document.createElement("button");
        join_button.id = "popup_join_group_button";
        if (this.groups[group.id])
        {
            join_button.innerHTML = "Joined";
            join_button.classList.add("joined");
        }
        else
        {
            join_button.innerHTML = "Join";
            join_button.addEventListener("click", (event) => {
                join_button.innerHTML = "Joining...";
                join_button.classList.add("joining");
                join_button.setAttribute("joining_group", group.id)

                const packet = {
                    type: "join_group",
                    group_id: group.id
                };

                this.server.ws_send(packet);
            });
        }
        group_div.appendChild(join_button);

        let group_list = document.getElementById("popup_group_list_con");
        group_list.appendChild(group_div);
    }

    start_popup_join_group_style_modifiers()
    {
        let groups = document.querySelectorAll(".popup_group");
        groups.forEach(group => {
            group.addEventListener("mouseenter", () => {
                let join_button = group.querySelector("#popup_join_group_button");
                if (!join_button.classList.contains("joined"))
                    join_button.classList.add("active");

            });
            group.addEventListener("mouseleave", () => {
                let join_button = group.querySelector("#popup_join_group_button");
                join_button.classList.remove("active");
            });
        });
    }

    stop_popup()
    {
        this.popup_container.style.display = "none";
        this.popup_create_group.style.display = "none";
        this.popup_join_group.style.display = "none";
        this.popup_settings.style.display = "none";
        this.popup_join = false;
    }

    set_input_height(lines)
    {
        this.input_pixels = this.input_pixels_default + (this.input_add_pixels * lines);

        if (this.input_pixels > this.input_max_height)
            this.input_pixels = this.input_max_height;

        this.main_element.style.gridTemplateRows = "auto " + this.input_pixels + "px";
    }

    increate_input_lines()
    {
        this.input_pixels += this.input_add_pixels;

        if (this.input_pixels > this.input_max_height)
            this.input_pixels = this.input_max_height;

        this.main_element.style.gridTemplateRows = "auto " + this.input_pixels + "px";
    }

    decrease_input_lines()
    {
        this.input_pixels -= this.input_add_pixels;

        if (this.input_pixels > this.input_max_height)
            this.input_pixels = this.input_max_height;

        this.main_element.style.gridTemplateRows = "auto " + this.input_pixels + "px";
    }

    send_message()
    {
        const msg = this.input_msg.value;
        if (msg == "")
            return;

        const packet = {
            type: "group_msg",
            group_id: this.current_group.id,
            content: msg
        };

        this.server.ws_send(packet);

        // create_msg_box(user, msg);
        
        // console.log("Sending msg:", msg);

        this.input_msg.value = "";
    }

} // class App

export let app = new App();

// const upload_file = document.getElementById("upload_pfp");
// upload_file.addEventListener('change', (event) => {
//     let file = event.target.files[0];
//     console.log(">>>>", file);

//     const reader = new FileReader();
//     reader.onload = function(event) {
//         const data = event.target.result;

//         fetch("/img/" + file.name, {
//             method: 'POST',
//             body: data
//         });
//     };

//     // reader.readAsBinaryString(file);
//     reader.readAsArrayBuffer(file);
// })
