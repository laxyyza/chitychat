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
    channels: Map<number, HubTextChannel>;
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
    memberIDs: Set<number>;
    createdAt: string;
    detailedLoaded: boolean;
    selectedChannelID: number;
    channelCategoryID: number;

    constructor(data: HubBasicData, categoryData?: CategoryData[], memberIDs?: number[]) {
        this.id = data.hub_id;
        this.ownerID = data.owner_id;
        this.name = data.name;
        this.pfp = null;
        this.selectedChannelID = -1;
        this.channelCategoryID = -1;

        if (categoryData) {
            this.categories = new Map(categoryData.map((cat) => (
                [
                    cat.category_id,
                    {
                        id: cat.category_id,
                        name: cat.name,
                        position: cat.position,
                        channels: new Map(cat.channels.map((channel) => (
                            [
                                channel.channel_id,
                                {
                                    id: channel.channel_id,
                                    hubID: channel.hub_id,
                                    categoryID: channel.category_id,
                                    name: channel.name,
                                    position: channel.position,
                                    createdAt: channel.created_at,
                                    messagesLoaded: false,
                                    messages: new Map(),
                                    offset: 0
                                }
                            ]
                        )))
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
        const cat = this.categories.get(this.channelCategoryID);
        if (!cat) return [];

        const channel = cat.channels.get(this.selectedChannelID);
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
        let channel: HubTextChannel | undefined = undefined;
        newHub.categories.forEach((cat) => {
            if (channel)
                return;
            channel = cat.channels.get(resp.channel_id);
        });
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

        newHub.categories.forEach((cat) => {
            const channel = cat.channels.get(msg.channel_id);
            if (channel) {
                channel.messages.set(msg.msg_id, {
                    id: msg.msg_id,
                    user_id: msg.user_id,
                    channel_id: msg.channel_id,
                    content: msg.content,
                    timestamp: msg.timestamp,
                    channel_type: 'hub',
                    attachments: msg.attachments
                })
            }
        });
        return newHub;
    }
    
}

export { Hub };
