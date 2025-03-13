import Message from "./message";

class Group 
{
    id: number;
    owner_id: number;
    name: string;
    created_at: string;
    public: boolean;
    desc?: string;
    messages: Message[];
    memberIDs: number[];

    constructor(
        id: number,
        owner_id: number,
        name: string,
        created_at: string,
        is_public: boolean
    ) {
        this.id = id;
        this.owner_id = owner_id;
        this.name = name;
        this.created_at = created_at;
        this.public = is_public;
        this.desc = '';
        this.messages = [];
        this.memberIDs = [this.owner_id];
    }
}

export default Group;