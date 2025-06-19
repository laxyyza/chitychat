import { Action, DispatchAction } from "../components/chatapp/AppProvider";
import fetchData from "../services/api";
import Message from "./message";

// Used for HTTP GET /api/hubs
export interface HubBasicData {
    hub_id: number;
    owner_id: number;
    name: string;
    public: boolean;
    created_at: string;
    settings: any;
}

export interface HubChannelData {
    channel_id: number;
    category_id: number;
    hub_id: number;
    name: string;
    position: number;
    created_at: string;
    settings: any;
}

export interface HubTextChannel {
    id: number;
    hubID: number;
    categoryID: number;
    name: string;
    position: number;
    createdAt: string;
    messagesLoaded: boolean;
    messages: Map<number, Message>;
    offset: number;
}

export interface Category {
    id: number;
    name: string;
    position: number;
    channelIDs: Set<number>;
}

export interface HubMessageData {
    msg_id: number;
    user_id: number;
    content: string;
    attachments: any;
    timestamp: string;
    hub_id: number;
    channel_id: number;
}

export interface CategoryData {
    category_id: number;
    name: string;
    position: number;
    channels: HubChannelData[];
}

export interface NewCategoryData {
    category_id: number;
    hub_id: number;
    name: string;
    position: number;
}

export interface HubDetailedData {
    hub_id: number;
    owner_id: number;
    name: string;
    public: boolean;
    created_at: string;
    settings: any;
    member_ids: number[];
    categories: CategoryData[];
}

export interface GetChannelMessagesData {
    hub_id: number;
    channel_id: number;
    messages: HubMessageData[];
}

class Hub {
    readonly id: number;
    readonly ownerID: number;
    name: string;
    pfp: string | null;
    categories: Map<number, Category>;
    channels: Map<number, HubTextChannel>;
    memberIDs: Set<number>;
    createdAt: string;
    detailedLoaded: boolean;
    selectedChannelID: number;

    constructor(data: HubBasicData, categoryData?: CategoryData[], memberIDs?: number[]) {
        this.id = data.hub_id;
        this.ownerID = data.owner_id;
        this.name = data.name;
        this.pfp = null;
        this.selectedChannelID = -1;
        this.channels = new Map();

        if (categoryData) {
            this.categories = new Map(categoryData.map((cat) => (
                [
                    cat.category_id,
                    {
                        id: cat.category_id,
                        name: cat.name,
                        position: cat.position,
                        channelIDs: new Set(cat.channels.map((channel) => {
                            this.channels.set(channel.channel_id, Hub.newChannel(channel))
                            return channel.channel_id;
                        }))
                    }
                ]
            )))
        } else {
            this.categories = new Map();
        }

        this.memberIDs = new Set(memberIDs || []);
        this.createdAt = data.created_at;
        this.detailedLoaded = false;
    }

    clone(): Hub {
        return Object.assign(Object.create(Object.getPrototypeOf(this)), this);
    }

    fetchChannelMessages(channel: HubTextChannel, dispatch: React.Dispatch<DispatchAction>, limit: number = 20) {
        fetchData(`/api/hubs/${this.id}/channels/${channel.id}/messages?limit=${limit}&offset=${channel.offset}`)
            .then((resp: GetChannelMessagesData) => {
                dispatch({type: Action.ADD_HUB_MSGS, payload: resp})
                channel.offset += limit;
            })
    }

    getMessages(dispatch?: React.Dispatch<DispatchAction>): Message[] {
        const channel = this.channels.get(this.selectedChannelID);
        if (!channel) return [];
        if (dispatch && channel.messagesLoaded === false) {
            this.fetchChannelMessages(channel, dispatch);
            channel.messagesLoaded = true;
        }

        return Array.from(channel.messages.values());
    }

    static fromDetailedData(data: HubDetailedData): Hub {
        const hub = new Hub({ ...data }, data.categories, data.member_ids)
        hub.detailedLoaded = true;
        return hub;
    }

    static fromGetMessages(hub: Hub, resp: GetChannelMessagesData): Hub {
        const newHub = hub.clone();
        const channel = newHub.channels.get(resp.channel_id);
        if (!channel)
            return hub;

        resp.messages.forEach((msg) => {
            channel?.messages.set(msg.msg_id, {
                id: msg.msg_id,
                channel_id: resp.channel_id,
                user_id: msg.user_id,
                attachments: msg.attachments,
                channel_type: 'hub',
                content: msg.content,
                timestamp: msg.timestamp
            })
        })

        return newHub;
    }

    static fromMessage(hub: Hub, msg: HubMessageData): Hub {
        const newHub = hub.clone();
        const channel = newHub.channels.get(msg.channel_id);
        channel?.messages.set(msg.msg_id, {
            id: msg.msg_id,
            user_id: msg.user_id,
            channel_id: msg.channel_id,
            content: msg.content,
            timestamp: msg.timestamp,
            channel_type: 'hub',
            attachments: msg.attachments
        });
        return newHub;
    }
    
    static fromAddChannel(hub: Hub, c: HubChannelData): Hub {
        const newHub = hub.clone();
        newHub.categories.get(c.category_id)?.channelIDs.add(c.channel_id);
        newHub.channels.set(c.channel_id, Hub.newChannel(c));
        return newHub;
    }
    
    static fromAddCategory(hub: Hub, c: NewCategoryData): Hub {
        const newHub = hub.clone();
        newHub.categories.set(c.category_id, {
            id: c.category_id,
            name: c.name,
            position: c.position,
            channelIDs: new Set(),
        })
        return newHub;
    }

    static newChannel(c: HubChannelData): HubTextChannel {
        return {
            id: c.channel_id,
            hubID: c.hub_id,
            categoryID: c.category_id,
            name: c.name,
            position: c.position,
            createdAt: c.created_at,
            messagesLoaded: false,
            messages: new Map(),
            offset: 0
        };
    }

    static fromAddMember(hub: Hub, userID: number): Hub {
        const newHub = hub.clone();
        newHub.memberIDs.add(userID);
        return newHub;
    }
}


export { Hub };
