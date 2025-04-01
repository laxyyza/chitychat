import Group from "./group";
import Message from "./message";

class DM {
	targetUserID: number;
	messages: Map<number, Message>;

	constructor(userID: number) {
		this.targetUserID = userID;
		this.messages = new Map();
	}

    clone(): DM {
        return Object.assign(Object.create(Object.getPrototypeOf(this)), this);
    }

    static fromAddMessages(dm: DM, msgs: Message[]): DM {
        const newDM = dm.clone();
        newDM.messages = new Map(dm.messages);
        msgs.forEach((msg) => {
            newDM.messages.set(msg.id, msg);
        });
        newDM.targetUserID = dm.targetUserID;

        return newDM;
    }
}

class DMChat {
	id: string;
	chat: DM | Group;
    requestMsgs: boolean;
    msgOffset: number;

	constructor(chat: DM | Group) {
		if (chat instanceof DM) {
			this.id = DMChat.DmID(chat.targetUserID);
		} else {
			this.id = DMChat.GroupID(chat.id);
		}
		this.chat = chat;
        this.requestMsgs = false;
        this.msgOffset = 0;
	}

	getGroup(): Group | undefined {
		if (this.chat instanceof Group)
			return this.chat;
		return undefined;
	}

	getDM(): DM | undefined {
		if (this.chat instanceof DM)
			return this.chat;
		return undefined;
	}

	static GroupID(id: number): string {
		return "group-" + id;
	}

	static DmID(id: number): string {
		return "dm-" + id;
	}

    static From(dmchat: DMChat, chat: DM | Group): DMChat {
        const newDMChat = new DMChat(chat);
        newDMChat.id = dmchat.id;
        newDMChat.requestMsgs = dmchat.requestMsgs;
        newDMChat.msgOffset = dmchat.msgOffset;

        return newDMChat;
    }
}

export { DM, DMChat };
