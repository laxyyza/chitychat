import { Action, DispatchAction } from "../components/chatapp/AppProvider";
import fetchData from "../services/api";
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
    lastMessage: string;

	constructor(chat: DM | Group, lastMessage: string) {
		if (chat instanceof DM) {
			this.id = DMChat.DmID(chat.targetUserID);
		} else {
			this.id = DMChat.GroupID(chat.id);
		}
		this.chat = chat;
        this.requestMsgs = false;
        this.msgOffset = 0;
        this.lastMessage = lastMessage;
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
        const newDMChat = new DMChat(chat, dmchat.lastMessage);
        newDMChat.id = dmchat.id;
        newDMChat.requestMsgs = dmchat.requestMsgs;
        newDMChat.msgOffset = dmchat.msgOffset;

        return newDMChat;
    }

    private async fetchGroupMessages(group: Group, limit: number): Promise<Message[]> {
        return fetchData(`/api/groups/${group.id}/messages?limit=${limit}&offset=${this.msgOffset}`)
            .then(data => {
                const messagesJson: any[] = data.messages;
                const messages: Message[] = [];

                messagesJson.forEach((msgJson) => {
                    const msg: Message = {
                        id: msgJson.msg_id,
                        user_id: msgJson.user_id,
                        channel_id: msgJson.channel_id,
                        channel_type: "dm",
                        content: msgJson.content,
                        attachments: msgJson.attachments,
                        timestamp: msgJson.timestamp
                    };

                    messages.push(msg);
                })
                return messages;
            })
            .catch(() => {
                return [];
            })
    }

    private async fetchDMMessages(dm: DM, limit: number): Promise<Message[]> {
        return fetchData(`/api/dms/${dm.targetUserID}?limit=${limit}&offset=${this.msgOffset}`)
            .then(data => {
                const messagesJson: any[] = data.messages;
                const messages: Message[] = [];

                messagesJson.forEach((msgJson) => {
                    const msg: Message = {
                        id: msgJson.msg_id,
                        user_id: msgJson.user_id,
                        channel_id: msgJson.channel_id,
                        channel_type: "dm",
                        content: msgJson.content,
                        attachments: msgJson.attachments,
                        timestamp: msgJson.timestamp
                    };

                    messages.push(msg);
                })
                return messages;
            })
            .catch(() => {
                return [];
            })
    }

    fetchMessages(dispatch: React.Dispatch<DispatchAction>, limit: number = 20) {
        var promise: Promise<Message[]> | undefined
        if (this.chat instanceof Group) {
            promise = this.fetchGroupMessages(this.chat, limit);
        } else if (this.chat instanceof DM) {
            promise = this.fetchDMMessages(this.chat, limit);
        }

        promise?.then((messages) => {
            if (messages.length > 0) {
                this.msgOffset += limit;
                dispatch({ type: Action.ADD_DM_MSGS, payload: { dmID: this.id, messages: messages } });
            }
        })
    }
}

export { DM, DMChat };
