export class User
{
    constructor(id, username, displayname, bio, created_at, pfp_name)
    {
        this.id = id;
        this.username = username;
        this.displayname = displayname;
        this.bio = bio;
        this.created_at = created_at;
        this.pfp_name = pfp_name;
        this.pfp_url = location.protocol + "//" + location.host + "/upload/imgs/" + pfp_name;
    }
}
