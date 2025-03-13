interface Message {
    id: number;
    user_id: number;
    channel_id: number;
    channel_type: 'hub' | 'group';
    content?: string;
    attachments?: string[];
}

export default Message;