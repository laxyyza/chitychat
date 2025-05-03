import UserIcon from './UserIcon';
import Message from '../../models/message';
import { useApp } from './AppProvider';
import Text from './Text';

interface Prop {
    message: Message;
}

interface ImgProp {
    url: string;
}

interface VideoProp {
    url: string;
}

interface AttachmentProp {
    url: string;
    type: 'video' | 'image';
}

const Img = ({ url }: ImgProp) => {
    return (
        <img
            className="rounded-3xl p-2 object-contain max-h-100 shrink"
            src={url}
        />
    );
};

const Vid = ({ url }: VideoProp) => {
    return (
        <video className="rounded-xl" width="600" controls>
            <source src={url} />
        </video>
    );
};

const Attachment = ({ url, type }: AttachmentProp) => {
    if (type === 'video') return <Vid url={url} />;
    else if (type === 'image') return <Img url={url} />;
    else return <h1>Unknown type: {type}</h1>;
};

const Timestamp = (timestamp: string) => {
    const date = new Date(timestamp);
    const now = new Date();

    // Check if it's today
    const isToday = date.toDateString() === now.toDateString();

    if (isToday) {
        const timeString = date.toLocaleTimeString('en-US', {
            hour: 'numeric',
            minute: '2-digit',
            hour12: true
        });
        return `Today at ${timeString}`;
    } else {
        // Fallback for other dates
        return date.toLocaleString('en-US', {
            weekday: 'long',
            year: 'numeric',
            month: 'long',
            day: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            hour12: true
        });
    }
};

const MessageComponent = ({ message }: Prop) => {
    const { app } = useApp();
    const user = app.users.get(message.user_id);

    if (!user) return null;

    return (
        <div className="relative m-2 rounded-2xl p-2 hover:bg-gray-700 text-white flex max-w-full">
            <div className='h-full'>
                <UserIcon user={user} size='24'></UserIcon>
            </div>
            <div className='pl-2'>
                <div className="">
                    <div className="flex-1">
                        <span className="font-bold pb-0 text-[16px] align-middle">
                            {user.displayname}
                        </span>
                        <span className="ml-4 text-xs text-gray-400 font-normal align-middle">
                            {Timestamp(message.timestamp)}
                        </span>
                    </div>
                </div>
                <div className="whitespace-pre-wrap text-[14px]">
                    <Text>{message.content}</Text>
                </div>
                <div className="flex flex-wrap">
                    {message.attachments &&
                        message.attachments.map((url) => (
                            <Attachment
                                url={url}
                                type={url.endsWith('.mp4') ? 'video' : 'image'}
                            ></Attachment>
                        ))}
                </div>
            </div>
        </div>
    );
};

export default MessageComponent;
