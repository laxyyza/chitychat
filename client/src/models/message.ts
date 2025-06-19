interface Message {
    id: number;
    user_id: number;
    channel_id: number;
    channel_type: 'hub' | 'group' | 'dm';
    content: string;
    attachments: string[];
    timestamp: string;
}

export default Message;
