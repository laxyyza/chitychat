interface Prop {
    tooltip: string;
    pfp: string | null;
    selected: boolean;
    children: string;
}

interface Message {
    id: number;
    user_id: number;
    channel_id: number;
    content?: string;
    attachments?: string[];
}

interface BaseChannel {
    id: number;
    name: string;
    hub_id: number;
}

export enum ChannelType {
    TEXT,
    VOICE
}

interface TextChannel extends BaseChannel {
    type: ChannelType.TEXT;
    messages: Message[];
}

interface VoiceChannel extends BaseChannel {
    type: ChannelType.VOICE;
}

type Channel = TextChannel | VoiceChannel;

interface Category {
    id: number;
    name: string;
    channelIDs: number[];
}

interface Group {
    id: number;
    name: string;
    messages: Message[];
}

export class Hub {
    readonly id: number;
    readonly owner_id: number;
    name: string;
    pfp: string | null;
    categories: Category[];
    channelIDs: number[];
    memberIDs: number[];
    channelIDIndex: number;

    constructor(
        id: number,
        owner_id: number,
        name: string,
        pfp: string | null = null,
        channelIDs: number[] = []
    ) {
        this.id = id;
        this.owner_id = owner_id;
        this.name = name;
        this.pfp = pfp;
        this.categories = [
            { id: 1, name: 'Text Channels', channelIDs: channelIDs }
        ];

        this.memberIDs = [this.owner_id];
        this.channelIDs = channelIDs;
        this.channelIDIndex = 0;
    }
}

export type { Group, TextChannel, Channel, Message, Category };
