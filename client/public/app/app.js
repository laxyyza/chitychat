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
            type: this.type,
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

        init_packet_commads();
        this.server = new Server(this);
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
        // console.log("type: " + file.type);

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
        this.popup_image.style.display = "none";
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

        let packet = {
            type: "group_msg",
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
