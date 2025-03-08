import { ReactNode, useEffect, useState } from 'react';
import Input from './Input';
import MessageComponent from './Message';
// import User from './User';
import { useApp } from './AppProvider';
import { ChannelType, Hub, Message, TextChannel } from './Hub';
import { BsEmojiTear } from 'react-icons/bs';

interface ChatWindowProp {
    children: ReactNode[];
}

interface HeaderBarProp {
    name: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-800 text-center text-white shadow">{name}</div>
    );
};

const ChatWindow = ({ children }: ChatWindowProp) => {
    return <div className="flex-1 overflow-y-auto">{children}</div>;
};

const MainContent = () => {
    const { app } = useApp();
    const [messages, setMessages] = useState<Message[]>([]);

    useEffect(() => {
        const hub: Hub = app.hubs[app.hubIndex];
        if (!hub) return;

        const channel = hub.channels.get(app.selectedChannelID);
        if (channel && channel.type === ChannelType.TEXT) {
            setMessages(channel.messages);
        } else {
            setMessages([]);
        }
    }, [app.selectedChannelID]);

    return (
        <div className="flex flex-col flex-1 h-screen bg-gray-700">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow>
                {messages.map((msg) => (
                    <MessageComponent
                        user={app.users.get(msg.user_id)}
                        content={msg.content}
                    ></MessageComponent>
                ))}
            </ChatWindow>
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] overflow-auto">
                <Input></Input>
            </div>
        </div>
    );
};

export default MainContent;
