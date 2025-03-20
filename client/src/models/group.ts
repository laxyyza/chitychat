import Message from "./message";

class Group {
    id: number;
    owner_id: number;
    name: string;
    created_at: string;
    public: boolean;
    desc?: string;
    messages: Map<number, Message>;
    memberIDs: number[];
    public detailsLoaded: boolean;
    public msgOffset: number;
    scrollTop: number;

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
        this.messages = new Map();
        this.memberIDs = [this.owner_id];
        this.detailsLoaded = false;
        this.msgOffset = 0;
        this.scrollTop = -1;
    }

    addMemberIDs(newMemberIDs: number[]) {
        this.memberIDs = [...new Set<number>([...this.memberIDs, ...newMemberIDs])];
    }

    clone(): Group {
        return Object.assign(Object.create(Object.getPrototypeOf(this)), this);
    }

    static fromAddMessage(group: Group, msg: Message): Group {
        const newGroup = group.clone();
        newGroup.messages = new Map(group.messages).set(msg.id, msg);
        newGroup.memberIDs = group.memberIDs;
        newGroup.detailsLoaded = group.detailsLoaded;

        return newGroup;
    }

    static loadMessages(group: Group, msgs: Message[]) {
        const newGroup = group.clone();
        newGroup.messages = new Map(group.messages)
        msgs.forEach(msg => newGroup.messages.set(msg.id, msg));
        newGroup.memberIDs = group.memberIDs;
        newGroup.detailsLoaded = group.detailsLoaded;
        newGroup.msgOffset = group.msgOffset + msgs.length;

        return newGroup;
    }
}

export default Group;