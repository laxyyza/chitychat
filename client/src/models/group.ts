import Message from "./message";

class Group {
    id: number;
    owner_id: number;
    name: string;
    created_at: string;
    public: boolean;
    desc?: string;
    messages: Message[];
    memberIDs: number[];
    public detailsLoaded: boolean;
    public msgOffset: number;

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
        this.detailsLoaded = false;
        this.msgOffset = 0;
    }

    addMemberIDs(newMemberIDs: number[]) {
        this.memberIDs = [...new Set<number>([...this.memberIDs, ...newMemberIDs])];
    }

    static fromAddMessage(group: Group, msg: Message): Group {
        const newGroup = new Group(group.id, group.owner_id, group.name, group.created_at, group.public);
        newGroup.messages = [...group.messages, msg];
        newGroup.memberIDs = group.memberIDs;
        newGroup.detailsLoaded = group.detailsLoaded;

        return newGroup;
    }

    static loadMessages(group: Group, msgs: Message[]) {
        const newGroup = new Group(group.id, group.owner_id, group.name, group.created_at, group.public);
        newGroup.messages = [...group.messages, ...msgs];
        newGroup.memberIDs = group.memberIDs;
        newGroup.detailsLoaded = group.detailsLoaded;
        newGroup.msgOffset = group.msgOffset + msgs.length;

        return newGroup;
    }
}

export default Group;