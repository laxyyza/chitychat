import { ReactNode } from 'react';
import Input from './Input';
import Message from './Message';
import User from './User';

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
    const user: User = {
        id: 51,
        username: 'username',
        displayname: 'Display Name',
        created_at: '16 December 2025, 5:15 pm',
        about_me: 'ABOUT ME',
        pfp: 'https://www.oola.com/wp-content/uploads/2022/07/communityIcon_x4lqmqzu1hi81.jpeg'
    };

    const messages = [
        'Message test',
        'Message test',
        'Message test',
        'Message test',
        'Message test',
        'Message test',
        'Message test',
        'Message ok',
        'Message ok',
        'Message ok',
        'Message ok',
        'Message ok',
        'Message ok',
        'Message ok',
        'Message ok'
    ];

    return (
        <div className="flex flex-col flex-1 h-screen bg-gray-700">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow>
                {messages.map((msg, index) => (
                    <Message user={user} content={msg}></Message>
                ))}
            </ChatWindow>
            <div className="bg-gray-900 m-3 rounded-2xl text-white max-h-[50%] overflow-auto">
                <Input></Input>
            </div>
        </div>
    );
};

export default MainContent;
