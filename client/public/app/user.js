import {app} from './app.js';
import { Group } from './group.js';

export class User
{
    constructor(id, username, displayname, bio, created_at, pfp_name, status)
    {
        this.id = id;
        this.username = username;
        this.displayname = displayname;
        this.bio = bio;
        this.created_at = created_at;
        if (!status)
            this.status = "offline";
        else
            this.status = status;

        if (!pfp_name)
            pfp_name = "default.jpg";

        this.update_pfp(pfp_name);
    }

    set_status(status)
    {
        this.status = status;

        for (let group_id in app.groups)
        {
            /**
             * @type {Group}
             */
            let group = app.groups[group_id];
            let group_member = group.div_group_members.querySelector("[group_member_id=\"" + 
                                                                    this.id + "\"]");
            if (group_member)
            {
                let member_status = group_member.querySelector("[member_status]");
                member_status.setAttribute("member_status", status);
            }
        }
    }

    update_pfp(pfp_name, rtusm)
    {
        this.pfp_url = location.protocol + "//" + location.host + "/upload/imgs/" + pfp_name;
        this.pfp_name = pfp_name;

        if (!rtusm)
            return;

        if (this.id === app.client_user.id)
        {
            let img_ele = document.getElementById("settings_img");
            img_ele.src = this.pfp_url;
        }

        for (let group_id in app.groups)
        {
            /**
             * @type {Group}
             */
            let group = app.groups[group_id];
            let group_member = group.div_group_members.querySelector("[group_member_id=\"" + 
                                                                    this.id + "\"]");
            
            if (group_member)
            {
                let member_img = group_member.getElementsByClassName("member_img")[0];

                member_img.src = this.pfp_url;
            }

            let msgs = group.div_chat_messages.querySelectorAll(
                "[msg_user_id=\"" + this.id + "\"]"
            );

            msgs.forEach(msg => {
                let msg_img = msg.getElementsByClassName("msg_img")[0];
                msg_img.src = this.pfp_url;
            });
        }
    }
}
