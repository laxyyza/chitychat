import {Server} from './server.js';
import {handle_packet_state, init_packet_commads} from './handle_packet.js'
// import {socket} from './ws.js';

class Attach 
{
    constructor(file, img=null)
    {
        this.file = file;
        this.name = file.name;
        this.type = file.type;
        this.size = file.size;
        
        const reader = new FileReader();
        reader.onload = (e) => {
            this.src = e.target.result;
            if (img)
                img.src = this.src;
        };
        reader.readAsDataURL(file);
    }

    tojson()
    {
        let f = {
            cmd: this.type,
            name: this.name
        };
        return JSON.stringify(f);
    }
}

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
        this.settings_username = document.getElementById("settings_username");
        this.settings_displayname = document.getElementById("settings_displayname");
        this.add_attachment_button = document.getElementById("msg_upload_button");
        this.popup_image = document.getElementById("popup_image");
        this.current_attachments = [];
        this.popup_create_group_public = document.getElementById("popup_create_group_public");
        this.group_info_button = document.getElementById("group_info");
        this.group_info_name = document.getElementById("group_info_name")
        this.group_desc = document.getElementById("group_desc");
        this.group_members_info = document.getElementById("group_members_info");
        this.group_info_arrow = document.getElementById("group_info_arrow");
        this.group_info_dropdown = document.getElementById("group_info_dropdown");
        this.popup_group_invite = document.getElementById("popup_group_invite");
        this.group_current_invite_list = document.getElementById("group_current_invite_list");
        this.popup_group_invite_header = document.getElementById("popup_group_invite_header");
        this.gen_group_code_button = document.getElementById("generate_group_code");
        this.gen_group_code_checkbox = document.getElementById("group_invite_checkbox");
        this.gen_group_code_max_uses = document.getElementById("max-uses-input");
        this.join_group_viacode_button = document.getElementById("join_group_code_button");

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

        this.join_group_viacode_button.addEventListener("click", () => {
            let input = document.getElementById("join_group_code_input");
            const code = input.value;
            if (code)
            {
                const packet = {
                    cmd: "join_group_code",
                    code: code
                };
                this.server.ws_send(packet);
            }
        });

        this.gen_group_code_button.addEventListener("click", () => {
            let max_uses;
            let group_id;

            if (!this.current_group)
                return;
            else
                group_id = this.current_group.id;

            if (this.gen_group_code_checkbox.checked)
                max_uses = -1;
            else
                max_uses = this.gen_group_code_max_uses.value;

            const packet = {
                cmd: "create_group_code",
                max_uses: Number(max_uses),
                group_id: group_id
            }
            this.server.ws_send(packet);
        });

        this.group_info_dropdown.addEventListener("click", (e) => {
            if (e.target.tagName === "OPTION") 
            {
                switch (e.target.value) 
                {
                    case "invite":
                        this.start_popup_group_invite(); 
                        break;
                    case "settings":
                        this.start_popup_group_settings();
                        break;
                    default:
                        break;
                }
            }
        });

        this.group_info_button.addEventListener("click", () => {
            if (this.group_info_dropdown.style.display === "block")
            {
                this.group_info_dropdown.style.display = "none";
                this.group_info_arrow.className = "arrow adown";
            }
            else
            {
                this.group_info_dropdown.style.display = "block";
                this.group_info_arrow.className = "arrow aup";
            }
        });

        this.popup_container.addEventListener("click", (e) => {
            if (e.target == this.popup_container)
                this.stop_popup();
        });

        this.add_attachment_button.addEventListener("click", () => {
            const file_input = document.getElementById("msg_upload_button_input");

            const change_event = (event) => {
                const files = event.target.files;
                const file = files[0];
                console.log(file);
                this.add_attachment(file);
                file_input.removeEventListener("change", change_event);
            };
            file_input.addEventListener("change", change_event);
            file_input.click();
        });

        this.close_popup_button.addEventListener("click", () => {
            this.stop_popup();
        });

        this.create_group_button.addEventListener("click", () => {
            this.start_popup_create_group(400, 240);
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
                cmd: "edit_account",
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

        document.addEventListener("paste", (ev) => {
            let data = ev.clipboardData;

            if (data)
            {
                for (let i = 0; i < data.items.length; i++)
                {
                    let item = data.items[i];

                    if (item.type.startsWith("image/"))
                    {
                        this.add_attachment(item.getAsFile());
                    }
                }
            }
        });

        document.addEventListener("keydown", (ev) => {
            if (ev.key === "Escape")
                this.stop_popup();
        });

        init_packet_commads();
        this.server = new Server(this);
    }

    add_group_invite_list(code, uses, max_uses, group_id)
    {
        let con = document.createElement("div");
        con.className = "group_current_invite";

        let group_code = document.createElement("div");
        group_code.className = "group_invite";

        let copy_button = document.createElement("button");
        copy_button.innerHTML = "COPY";
        copy_button.title = "Copy " + code;
        copy_button.addEventListener("click", () => {
            navigator.clipboard.writeText(code);
            copy_button.title = "Copied";
        });

        let code_span = document.createElement("span");
        code_span.innerHTML = code;
        code_span.style.color = "yellow";

        group_code.appendChild(copy_button);
        group_code.appendChild(code_span);
        con.appendChild(group_code);

        let uses_span = document.createElement("span");
        uses_span.innerHTML = uses + " Users joined";
        con.appendChild(uses_span);

        let max_uses_div = document.createElement("div");
        let max_uses_span = document.createElement("span");
        max_uses_span.className = "invite_uses";
        max_uses_span.innerHTML = (max_uses === -1) ? "Infinite Uses" : max_uses + " Max Uses";

        let delete_code_button = document.createElement("button");
        delete_code_button.className = "delete_button";
        delete_code_button.innerHTML = "DELETE";
        delete_code_button.addEventListener("click", () => {
            const packet = {
                cmd: "delete_group_code",
                code: code,
                group_id: group_id
            };
            this.server.ws_send(packet);

            this.group_current_invite_list.removeChild(con);
        });

        max_uses_div.appendChild(max_uses_span);
        max_uses_div.appendChild(delete_code_button);

        con.appendChild(max_uses_div);

        this.group_current_invite_list.appendChild(con);
    }

    start_popup_group_invite()
    {
        this.popup_container.style.display = "block";
        this.popup.style.width = "700px";
        this.popup.style.height = "600px";
        this.popup_group_invite.style.display = "block";
        this.group_current_invite_list.innerHTML = "";
        this.popup_group_invite_header.innerHTML = this.current_group.name + " Invite Codes";

        const packet = {
            cmd: "get_group_codes",
            group_id: this.current_group.id
        };
        this.server.ws_send(packet);
    }

    start_popup_group_settings()
    {

    }

    start_popup_settings()
    {
        this.popup_settings.style.display = "block";
        this.popup_container.style.display = "block";
        this.popup.style.width = "70%";
        this.popup.style.height = "70%";
    }

    start_popup_image(img_src)
    {
        if (!img_src)
            return;

        let img = document.createElement("img");
        img.src = img_src;
        img.id = "popup_image_img";

        this.popup_container.style.display = "block";
        this.popup.style.width = "80%";
        this.popup.style.height = "80%";
        this.popup_image.style.display = "block";
        this.popup_image.appendChild(img);
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
            cmd: "session",
            id: Number(session_id)
        };

        this.server.ws_send(session_packet);
    }

    server_error()
    {

    }

    server_msg(packet)
    {
        console.log(packet);

        handle_packet_state(packet);
    }

    server_close(event)
    {
        this.logged_in = false;
    }

    add_attachment(file)
    {
        if (!file.type.startsWith("image/"))
        {
            console.log(file.name + " - is not image.");
            return;
        }

        let attach_con = document.createElement("div");
        attach_con.className = "input_attachment_con";
        attach_con.setAttribute("title", file.name);

        let close_button = document.createElement("button");
        close_button.className = "attachment_close";
        close_button.innerHTML = "<i class=\"fa fa-trash-o\"></i>";
        
        let img = document.createElement("img");
        img.className = "input_attachments_img";
        const attach_file = new Attach(file, img);
        console.log(attach_file.tojson());
        this.current_attachments.push(attach_file);
        const index = this.current_attachments.length - 1;
        // const reader = new FileReader();
        // reader.onload = (e) => {
        //     img.src = e.target.result;
        // };
        // reader.readAsDataURL(file);
        // console.log("cmd: " + file.type);

        let filename_span = document.createElement("span");
        filename_span.className = "attachment_name";
        filename_span.innerText = file.name;

        attach_con.appendChild(img);
        attach_con.appendChild(close_button);
        attach_con.appendChild(filename_span);

        let attachments = document.getElementById("input_attachments");
        attachments.appendChild(attach_con);

        close_button.addEventListener("click", () => {
            attachments.removeChild(attach_con);
            this.current_attachments.splice(index, 1);
        });
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

            const public_group = this.popup_create_group_public.checked;

            const packet = {
                cmd: "group_create",
                name: input.value,
                public: public_group
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
            cmd: "get_all_groups"
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
        if (this.groups[group.group_id])
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
                join_button.setAttribute("joining_group", group.group_id)

                const packet = {
                    cmd: "join_group",
                    group_id: group.group_id
                };
                console.log(packet);

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
        this.popup_image.style.display = "none";
        this.popup_group_invite.style.display = "none";
        this.popup_image.innerHTML = "";
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
        if (msg == "" && this.current_attachments.length === 0)
            return;

        if (!this.current_group)
        {
            console.warn("Group not selected.");
            return;
        }

        let packet = {
            cmd: "group_msg",
            group_id: this.current_group.id,
            content: msg,
            attachments: []
        };

        for (let i = 0; i < this.current_attachments.length; i++)
        {
            const att = this.current_attachments[i];
            packet.attachments.push({
                type: att.type,
                name: att.name
            });
        }

        this.server.ws_send(packet);

        // create_msg_box(user, msg);
        
        // console.log("Sending msg:", msg);

        this.input_msg.value = "";
    }

} // class App

export let app = new App();
