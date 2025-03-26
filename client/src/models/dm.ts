import Group from "./group";
import Message from "./message";

class DM {
	targetUserID: number;
	messages: Map<number, Message>;

	constructor(userID: number) {
		this.targetUserID = userID;
		this.messages = new Map();
	}
}

class DMChat {
	id: string;
	chat: DM | Group;

	constructor(chat: DM | Group) {
		if (chat instanceof DM) {
			this.id = DMChat.DmID(chat.targetUserID);
		} else {
			this.id = DMChat.GroupID(chat.id);
		}
		this.chat = chat;
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
}

export { DM, DMChat };