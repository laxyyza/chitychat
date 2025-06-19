import Message from "./message";

export interface GroupProps {
    group_id: number;
    owner_id: number;
    channel_id: number;
    name: string;
    desc: string;
    member_ids: number[];
    created_at: string;
    last_message: string;
}

class Group {
    id: number;
    owner_id: number;
    name: string;
    created_at: string;
    desc: string;
    messages: Map<number, Message>;
    memberIDs: Set<number>;
    public detailsLoaded: boolean;
    public msgOffset: number;
    scrollTop: number;

    constructor(
        id: number,
        owner_id: number,
        name: string,
        desc: string,
        created_at: string,
        memberIDs: number[]
    ) {
        this.id = id;
        this.owner_id = owner_id;
        this.name = name;
        this.created_at = created_at;
        this.desc = desc;
        this.messages = new Map();
        this.memberIDs = new Set(memberIDs);
        this.detailsLoaded = false;
        this.msgOffset = 0;
        this.scrollTop = -1;
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

    static addMemberIDs(group: Group, newMemberIDs: number[]): Group {
        const newGroup = group.clone();

        newMemberIDs.forEach(id => {
            newGroup.memberIDs.add(id);
        })

        return newGroup;
    }

    static delMemberID(group: Group, memberID: number): Group {
        const newGroup = group.clone();

        newGroup.memberIDs.delete(memberID);

        return newGroup;
    }
}

export default Group;
