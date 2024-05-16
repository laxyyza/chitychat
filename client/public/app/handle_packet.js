import {app} from './app.js';
import {Group} from './group.js';
import {User} from './user.js';

let packet_commands = {}

function cmd_session(packet)
{
    app.logged_in = true;

    const req_user_info = {
        cmd: "client_user_info"
    };
    const req_groups = {
        cmd: "client_groups"
    };

    app.server.ws_send(req_user_info);
    app.server.ws_send(req_groups);
}

function client_user_info(packet)
{
    app.client_user = new User(packet.user_id, packet.username, 
                                packet.displayname, packet.bio, 
                                packet.created_at, packet.pfp_name, 
                                packet.status);
    app.users[app.client_user.id] = app.client_user;

    let me_displayname = document.getElementById("me_displayname");
    me_displayname.innerHTML = app.client_user.displayname;
    let me_username = document.getElementById("me_username");
    me_username.innerHTML = app.client_user.username;
    app.img_ele.src = app.client_user.pfp_url;
    app.settings_displayname.innerHTML = app.client_user.displayname;
    app.settings_username.innerHTML = app.client_user.username;
}

function client_groups(packet)
{
    let urlparam = new URLSearchParams(window.location.search);
    let param_group_id = urlparam.get("group_id");
    let selected = false;
    const len = packet.groups.length;
    for (let i = 0; i < len; i++)
    {
        const group = packet.groups[i];
        const new_group = new Group(
            group.group_id, 
            group.owner_id, 
            group.name, 
            group.desc, 
        );
        app.groups[group.group_id] = new_group;

        if (!param_group_id)
            param_group_id = localStorage.getItem("group_id");

        if (len === 1)
        {
            new_group.select();
            selected = true;
        }
        else if (param_group_id)
        {
            param_group_id = Number(param_group_id);
            if (param_group_id === new_group.id)
            {
                new_group.select();
                selected = true;
            }
        }

        if (app.popup_join)
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

    if (!selected)
        localStorage.removeItem("group_id");
}

// function group_members(packet)
// {
//     let group = app.groups[packet.group_id];
//     if (!group)
//     {
//         console.warn("no group", packet.group_id);
//         return;
//     }
//     
//     for (let i = 0; i < packet.members.length; i++)
//     {
//         const user_id = packet.members[i];
//         group.members_id.push(user_id);
//         const user = app.users[user_id];
//         // const user = check_have_user(user_id);
//         if (user)
//         {
//             group.add_member(user);
//             continue;
//         }
//
//         const new_packet = {
//             cmd: "get_user",
//             user_id: user_id
//         };
//
//         app.server.ws_send(new_packet);
//     }
// }

function resolve_group_members(new_user)
{
    for (let group_id in app.groups)
    {
        let group = app.groups[group_id];
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
}

function resolve_popup_users(new_user)
{
    if (app.popup_join)
    {
        let owner_lists = document.querySelectorAll("#popup_group_owner" + new_user.id);
        let owner_username_lists = document.querySelectorAll("#popup_group_owner_username" + new_user.id);

        for (let i = 0; i < owner_lists.length; i++)
        {
            let span_owner = owner_lists[i];
            span_owner.innerHTML = new_user.displayname;
        }

        for (let i = 0; i < owner_username_lists.length; i++)
        {
            let span_owner_username = owner_username_lists[i];
            span_owner_username.innerHTML = new_user.username;
        }
    }
}

function resolve_group_msgs(new_user)
{
    for (let group_id in app.groups)
    {
        const group = app.groups[group_id];
        const div_msgs = group.div_chat_messages;
        const user_msgs = div_msgs.querySelectorAll("[msg_user_id=\"" + new_user.id + "\"]");
        user_msgs.forEach(msg => {
            let pfp = msg.querySelector(".msg_img");
            pfp.src = new_user.pfp_url;

            let name = msg.querySelector(".msg_displayname");
            name.innerHTML = new_user.displayname;
        });
    }
}

function get_user(users_packet)
{
    users_packet.users.forEach(user => {
        let new_user = new User(user.user_id, user.username, 
                                user.displayname, user.bio,
                                user.created_at, user.pfp_name,
                                user.status);
        app.users[new_user.id] = new_user;
        
        resolve_popup_users(new_user);
        resolve_group_members(new_user);
        resolve_group_msgs(new_user);
    });
}

function get_all_groups(packet)
{
    let requested_users = [];
    for (let i = 0; i < packet.groups.length; i++)
    {
        let group = packet.groups[i];
        let owner = app.users[group.owner_id];
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
                requested_users.push(group.owner_id);
        }

        app.popup_join_group_add_group(group);
    }

    const get_owners = {
        cmd: "get_user",
        user_ids: requested_users
    };
    app.server.ws_send(get_owners);

    app.start_popup_join_group_style_modifiers();
}

function group_msg(packet)
{
    let user = app.users[packet.user_id];
    let group = app.groups[packet.group_id];
    if (!user)
    {
        user = {
            displayname: toString(packet.user_id),
        };
    }

    group.add_msg(user, packet, false);
}

function get_group_msgs(packet)
{
    let group = app.groups[packet.group_id];

    for (let i = 0; i < packet.messages.length; i++)
    {
        const msg = packet.messages[i];
        let user = app.users[msg.user_id];
        if (!user)
        {
            user = {
                id: msg.user_id,
                displayname: ""
            };
        }

        group.add_msg(user, msg);
    }
}

function join_group(packet)
{                
    let group = app.groups[packet.group_id];
    let user = app.users[packet.user_id];
    group.members_id.push(packet.user_id);
    if (!user)
    {
        const get_user_packet = {
            cmd: "get_user",
            user_ids: [
                packet.user_id
            ]
        };

        app.server.ws_send(get_user_packet);
        return;
    }

    group.add_member(user);
}

function edit_account(packet)
{
    const upload_token = packet.upload_token;
    const file = app.new_pfp_file;

    console.log(file);

    const reader = new FileReader();
    reader.onload = (event) => {
        const data = event.target.result;

        fetch("/img/" + file.name, {
            method: 'POST',
            headers: {
                "Upload-Token": upload_token
            },
            body: data
        })
        .catch(error => {
            console.error("Failed to upload", error)
        });
    };

    reader.readAsArrayBuffer(file);
}

function send_attachments(packet)
{
    const token = packet.upload_token;
    const attachs = app.current_attachments;

    for (let i = 0; i < attachs.length; i++)
    {
        const file = attachs[i].file;
        const reader = new FileReader();

        reader.onload = (ev) => {
            const data = ev.target.result;

            fetch("/upload/imgs", {
                method: 'POST',
                headers: {
                    "Upload-Token": token,
                    "Attach-Index": i
                },
                body: data
            }).then(response => {
                console.log(response);
            })
            .catch(error => {
                console.error("POST error:", error);
            });

            app.current_attachments.splice(i, 1);
        };
        reader.readAsArrayBuffer(file);
    }
    
    let attachments = document.getElementById("input_attachments");
    while (attachments.firstChild)
    {
        attachments.removeChild(attachments.firstChild);
    }
    app.current_attachments.splice(0, app.current_attachments.length);
}

function handle_packet(packet)
{
    if (packet_commands.hasOwnProperty(packet.cmd))
        packet_commands[packet.cmd](packet);
    else if (packet.cmd === "error")
        console.error("ERROR: " + packet.error_msg);
    else
        console.error("No packet cmd: " + packet.type);
}

function handle_login(packet)
{
    if (packet.cmd === "session")
        packet_commands.session(packet);
    else
        window.location.href = "/login";
}

function group_codes(packet)
{
    for (let i = 0; i < packet.codes.length; i++)
    {
        let code = packet.codes[i];
        app.add_group_invite_list(code.code, code.uses, code.max_uses, packet.group_id);
    }
}

export function handle_packet_state(packet)
{
    if (app.logged_in)
        handle_packet(packet);
    else
        handle_login(packet);
}

function delete_msg(packet)
{
    const group_id = packet.group_id;
    const msg_id = packet.msg_id;

    let group = app.groups[group_id];
    
    let msg = group.div_chat_messages.querySelector("[msg_id=\"" + msg_id + "\"]");
    if (msg)
        msg.remove();
}

function rtusm(packet)
{
    /**
     * @type {User}
     */
    let user = app.users[packet.user_id];
    if (packet.status)
        user.set_status(packet.status);
    if (packet.pfp_name)
        user.update_pfp(packet.pfp_name, true);
}

function delete_group(packet)
{
    /**
     * @type {Group}
     */
    let group = app.groups[packet.group_id];

    if (!group)
        return;

    app.group_list.removeChild(group.div_list);

    if (app.current_group.id === group.id)
    {
        app.stop_popup();
        group.unselect();
    }
    
    delete app.groups[packet.group_id];
}

function get_member_ids(packet)
{
    const group_id = packet.group_id;
    let group = app.groups[group_id];
    let member_ids = packet.member_ids;

    group.members_id = member_ids;
    member_ids = member_ids.filter(user_id => {
        const ret = !app.users[user_id];
        if (ret === false)
            group.add_member(app.users[user_id]);
        return ret;
    });

    if (member_ids.length === 0)
        return;

    const get_user_packet = {
        cmd: "get_user",
        user_ids: member_ids
    }
    app.server.ws_send(get_user_packet)
}

export function init_packet_commads()
{
    packet_commands.session = cmd_session;

    packet_commands.client_user_info = client_user_info;
    packet_commands.client_groups = client_groups;
    packet_commands.get_member_ids = get_member_ids;
    // packet_commands.group_members = group_members;
    packet_commands.get_user = get_user;
    packet_commands.get_all_groups = get_all_groups;
    packet_commands.group_msg = group_msg;
    packet_commands.get_group_msgs = get_group_msgs;
    packet_commands.join_group = join_group;
    packet_commands.edit_account = edit_account;
    packet_commands.send_attachments = send_attachments;
    packet_commands.group_codes = group_codes;
    packet_commands.delete_msg = delete_msg;
    packet_commands.rtusm = rtusm;
    packet_commands.delete_group = delete_group;
}
