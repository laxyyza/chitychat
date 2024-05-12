// import {users, socket, selected_group} from './app.js';
import {app} from './app.js';

export class Group
{
    constructor(id, owner_id, name, desc)
    {
        this.name = name;
        this.id = Number(id);
        this.owner_id = Number(owner_id);
        this.members_id = [];
        this.desc = desc;

        this.div_chat_messages = document.createElement("div");
        this.div_chat_messages.id = "messages";

        this.div_group_members = document.createElement("div");
        this.div_group_members.id = "group_members_list";

        this.div_list = document.createElement("div");
        this.div_list.className = "group";
        this.div_list.id = "group_" + this.id;
        this.div_list.innerHTML = this.name;
        this.msg_offset = 0;
        this.get_scroll_messages = () => {
            if (app.messages_container.scrollTop === 0)
            {
                const packet = {
                    cmd: "get_group_msgs",
                    group_id: this.id,
                    limit: 15,
                    offset: this.msg_offset
                };
                this.msg_offset += packet.limit;

                app.server.ws_send(packet);
            }
        };
        this.div_list.addEventListener("click", () => {
            this.select();
        });

        this.members = [];
        this.messages = [];

        if (this.members_id)
        {
            for (let m = 0; m < this.members_id.length; m++)
            {
                const id = this.members_id[m];
                const user = app.users[id];
                if (user)
                    this.add_member(user);
                else
                {
                    const packet = {
                        cmd: "get_user",
                        user_id: id
                    };

                    app.server.ws_send(packet);
                }
            }
        }

        app.group_list.appendChild(this.div_list);
    }

    add_member(member)
    {
        let div_member = document.createElement("div");
        div_member.className = "group_member";
        div_member.setAttribute("group_member_id", member.id);

        let span_name = document.createElement("span");
        span_name.className = "member_name";
        span_name.innerText = member.displayname;

        let member_img = document.createElement("img");
        member_img.className = "member_img";
        member_img.src = member.pfp_url;

        let member_status = document.createElement("div");
        member_status.classList.add("member_status");
        member_status.setAttribute("member_status", member.status);

        div_member.appendChild(member_img);
        div_member.appendChild(member_status);
        div_member.appendChild(span_name);

        this.div_group_members.appendChild(div_member);
        this.members.push(member);
    }

    add_msg(user, msg, insert=true)
    {
        const content = msg.content;
        const attachments = msg.attachments;

        let div_msg = document.createElement("div");
        div_msg.className = "msg";
        div_msg.setAttribute("msg_user_id", user.id);
        div_msg.setAttribute("msg_id", msg.msg_id);

        let div_msg_top = document.createElement("div");
        div_msg_top.className = "msg_top";
        div_msg.appendChild(div_msg_top);

        let div_img = document.createElement("img");
        div_img.className = "msg_img";
        div_img.src = user.pfp_url;
        div_msg_top.appendChild(div_img);

        let span_displayname = document.createElement("span");
        span_displayname.className = "msg_displayname";
        span_displayname.innerHTML = user.displayname;
        div_msg_top.appendChild(span_displayname);

        let span_timestamp = document.createElement("span");
        span_timestamp.className = "msg_timestamp";
        span_timestamp.innerHTML = msg.timestamp;
        div_msg_top.appendChild(span_timestamp);

        let div_content = document.createElement("div");
        div_content.className = "msg_content_con";

        if (this.owner_id === app.client_user.id || 
            msg.user_id === app.client_user.id)
        {
            let delete_button = document.createElement("button");
            delete_button.className = "del_msg_button";
            delete_button.innerHTML = "<i class=\"fa fa-trash-o\"></i>";

            delete_button.addEventListener("click", () => {
                const packet = {
                    cmd: "delete_msg",
                    msg_id: msg.msg_id
                };

                app.server.ws_send(packet);
            });

            div_msg_top.appendChild(delete_button);
        }

        // let img = document.createElement("img");
        // img.setAttribute("src", "/img/default.png");
        // div_content.appendChild(img);
        // TODO: Add profile pics
        // new column in Users, called pfp_path, default "/img/default.png"
        // Add user settings to upload pfp

        let span_msg_content = document.createElement("div");
        span_msg_content.className = "msg_content";
        span_msg_content.innerHTML = content;
        div_content.appendChild(span_msg_content);
        div_msg.appendChild(div_content);

        let attach_con = document.createElement("div");
        attach_con.className = "msg_attach_con";

        for (let i = 0; i < attachments.length; i++)
        {
            const attch = attachments[i];
            if (attch.type.startsWith("image/"))
            {
                let div = document.createElement("div");
                div.className = "msg_attach_img_con";
                let img = document.createElement("img");
                img.src = location.protocol + "//" + location.host + "/upload/imgs/" + attch.hash;
                img.className = "msg_attach_img";
                img.title = attch.name;
                img.style.width = "450px";
                img.style.height = "450px";

                console.log("div max_width height: " + div.style.maxWidth);

                let spinner = document.createElement("div");
                spinner.className = "spinner";
                div.appendChild(spinner);

                img.onload = (ev) => {
                    img.style.width = "";
                    img.style.height = "";
                    div.style.backgroundColor = "inherit";
                    div.removeChild(spinner);
                };
                img.addEventListener("click", () => {
                    app.start_popup_image(img.src);
                });

                div.appendChild(img);
                attach_con.appendChild(div);
                // div_msg.appendChild(img);
            }
        }

        div_msg.appendChild(attach_con);

        // let messages = document.getElementById("messages");
        let messages = this.div_chat_messages;

        if (insert)
        {
            messages.insertBefore(div_msg, messages.firstChild);
            app.messages_container.scrollTop += div_msg.offsetTop;
        }
        else 
        {
            messages.appendChild(div_msg);
            app.messages_container.scrollTo(0, app.messages_container.scrollHeight);
        }

        app.set_input_height(0);

        this.messages.push(msg);
    }

    unselect()
    {
        if (app.current_group !== this)
            return;

        this.div_list.classList.remove("active");

        app.messages_container.removeChild(this.div_chat_messages);
        app.group_member_container.removeChild(this.div_group_members);

        app.current_group = null;
        app.selected_group = null;
    }

    get_member_ids()
    {
        const packet = {
            cmd: "get_member_ids",
            group_id: this.id
        };
        app.server.ws_send(packet)
    }

    select()
    {
        if (app.selected_group)
        {
            app.selected_group.classList.remove("active");
        }

        if (this.members_id.length == 0)
            this.get_member_ids()

        this.div_list.classList.add("active");

        app.selected_group = this.div_list;

        document.getElementsByClassName("messages");

        let current_messages = document.getElementById("messages");
        let current_group_list = document.getElementById("group_members_list");

        if (current_messages)
            app.messages_container.replaceChild(this.div_chat_messages, current_messages);
        else
            app.messages_container.appendChild(this.div_chat_messages);

        if (current_group_list)
            app.group_member_container.replaceChild(this.div_group_members, current_group_list);
        else
            app.group_member_container.appendChild(this.div_group_members);

        app.messages_container.scrollTo(0, app.messages_container.scrollHeight);

        if (app.current_group && app.current_group.get_scroll_messages)
            app.messages_container.removeEventListener('scroll', app.current_group.get_scroll_messages);
        app.current_group = this;

        if (this.messages.length === 0)
        {
            const packet = {
                cmd: "get_group_msgs",
                group_id: this.id,
                limit: 15,
                offset: this.msg_offset
            };
            this.msg_offset += packet.limit;

            app.server.ws_send(packet);
        }

        app.messages_container.addEventListener('scroll', this.get_scroll_messages)
        app.group_info_name.innerHTML = this.name;
        app.group_desc.innerHTML = this.desc;
        app.group_members_info.innerHTML = this.members.length + " members";
        localStorage.setItem("group_id", this.id);
    }
    // TODO: Add a `update_member()` method.
}
