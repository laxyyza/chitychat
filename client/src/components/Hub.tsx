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

const HubIcon = (name: string, pfp: string | null) => {
    if (pfp) {
        return <img className="custom-rounded-inherit" src={pfp} />;
    } else {
        let letters: string = '';
        const words = name.split(' ');
        const className = words.length >= 3 ? 'text-xl' : 'text-3xl';

        for (let i = 0; i < Math.min(3, words.length); i++) {
            letters += words[i][0];
        }

        return <div className={className}>{letters}</div>;
    }
};

export const HubComponent = ({ tooltip, selected, pfp, children }: Prop) => {
    return (
        <>
            {HubIcon(children, pfp)}
            <span className="hub-tooltip group-hover:scale-100 pointer-events-none">
                {tooltip}
            </span>
            <span
                className={`hub-highlight ${
                    selected ? 'hub-highlight-selected' : ''
                }`}
            ></span>
        </>
    );
};

export type { Group, TextChannel, Channel, Message, Category };
