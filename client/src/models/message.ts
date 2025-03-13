interface Message {
    id: number;
    user_id: number;
    channel_id: number;
    content?: string;
    attachments?: string[];
}

export default Message;