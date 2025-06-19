import Message from "./message";

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

export type {Channel, TextChannel, VoiceChannel, BaseChannel};
export {ChannelType};
