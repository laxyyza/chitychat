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
        this.status = status;

        if (!pfp_name)
            pfp_name = "default.jpg";

        this.pfp_name = pfp_name;
        this.pfp_url = location.protocol + "//" + location.host + "/upload/imgs/" + pfp_name;
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

            console.log("group_id", group_id);
            console.log(group_member);
            
            if (group_member)
            {
                let member_status = group_member.querySelector("[member_status]");
                member_status.setAttribute("member_status", status);
            }
        }
    }
}
