import { ReactNode } from 'react';
import { IoAddCircleOutline } from 'react-icons/io5';

interface MessageProp {
    username: string;
    content: string;
}

interface ChatWindowProp {
    children: ReactNode[];
}

interface HeaderBarProp {
    name: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return <div className="bg-green-900 text-center">{name}</div>;
};

const Message = ({ username, content }: MessageProp) => {
    return (
        <div className="bg-gray-700 m-2">
            <div>{username}</div>
            <div>{content}</div>
        </div>
    );
};

const ChatWindow = ({ children }: ChatWindowProp) => {
    return <div className="chat-window overflow-auto">{children}</div>;
};

const Input = () => {
    return (
        <div className="input flex">
            <IoAddCircleOutline className="" size="40"></IoAddCircleOutline>
            <input className="w-full h-full" type="text" />
        </div>
    );
};

const MainContent = () => {
    const messages = [
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />
    ];

    return (
        <div className="main-content">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow>{messages}</ChatWindow>
            <Input></Input>
        </div>
    );
};

export default MainContent;
