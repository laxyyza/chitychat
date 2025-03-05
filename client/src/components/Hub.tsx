import { FaBeer } from 'react-icons/fa';

interface Prop {
    hub: Hub;
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
}

enum ChannelType {
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
    channels: Channel[];
}

export class Hub {
    readonly id: number;
    readonly owner_id: number;
    name: string;
    pfp: string | null;
    categories: Category[];
    memberIDs: number[];

    constructor(
        id: number,
        owner_id: number,
        name: string,
        pfp: string | null = null
    ) {
        this.id = id;
        this.owner_id = owner_id;
        this.name = name;
        this.pfp = pfp;
        this.categories = [];
        this.memberIDs = [this.owner_id];
    }
}

const HubIcon = (hub: Hub) => {
    if (hub.pfp) {
        return <img className="hub-icon" src={hub.pfp} />;
    } else {
        let letters: string = '';
        const words = hub.name.split(' ');
        const className = words.length >= 3 ? 'text-2xl' : 'text-3xl';

        for (let i = 0; i < Math.min(3, words.length); i++) {
            letters += words[i][0];
        }

        return <div className={className}>{letters}</div>;
    }
};

export const HubComponent = ({ hub }: Prop) => {
    return (
        <div key={hub.id} className="hub-icon group">
            {HubIcon(hub)}

            <span className="hub-tooltip group-hover:scale-100">
                {hub.name}
            </span>
        </div>
    );
};

export type { TextChannel, Channel, Message };
