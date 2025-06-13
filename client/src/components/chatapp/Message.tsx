import UserIcon from './UserIcon';
import Message from '../../models/message';
import { useApp } from './AppProvider';
import Text from './Text';
import { useEffect, useState } from 'react';
import { FaFile } from 'react-icons/fa6';
import { FiDownload } from 'react-icons/fi';

interface Prop {
    message: Message;
}

interface ImgProp {
    url: string;
}

interface VideoProp {
    url: string;
}

interface FileProp {
    info: Attachment;
}

interface AttachmentProp {
    url: string;
}

interface Attachment {
    type: string;
    name: string;
    size: number;
    url: string;
}

const Img = ({ url }: ImgProp) => {
    const [maxHeight, setMaxHeight] = useState<'max-h-60' | 'max-h-140'>('max-h-60');

    return (
        <img
            className={`rounded-md transition-all m-1 object-contain shrink ${maxHeight}`}
            src={url}
            onClick={() => {
                setMaxHeight(maxHeight === 'max-h-60' ? 'max-h-140' : 'max-h-60');
            }}
        />
    );
};

const Vid = ({ url }: VideoProp) => {
    return (
        <video className="rounded-xl p-2" width="500" controls>
            <source src={url} />
        </video>
    );
};

const File = ({ info }: FileProp) => {
    function formatBytes(bytes: number) {
        if (bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        const size = bytes / Math.pow(k, i);
        return `${size.toFixed(1)} ${sizes[i]}`;
    }

    return (
        <div className='bg-gray-900 rounded-xl p-1 m-1 border-1 border-black flex items-center max-h-20 w-70 max-w-70'>
            <div className='mr-2 shrink-0'>
                <FaFile size={42} />
            </div>
            <div className='mr-2 overflow-hidden grow-1 text-ellipsis text-nowrap'>
                {info.name}
                <div className='text-xs'>
                    {formatBytes(info.size)}
                </div>
                <div className='text-xs text-gray-400'>
                    {info.type}
                </div>
            </div>
            <a href={info.url} download>
                <button
                    className='flex items-center p-2 hover:bg-gray-800 rounded-xl shrink-0'
                >
                    <FiDownload size={24} />
                </button>
            </a>
        </div>
    )
};

const Attachment = ({ url }: AttachmentProp) => {
    const [info, setInfo] = useState<Attachment | null>(null);

    const fetchHead = async (): Promise<Attachment> => {
        const resp = await fetch(url, {
            method: 'HEAD',
        });
        const type = resp.headers.get('Content-Type') || '';
        const size = parseInt(resp.headers.get('Content-length') || '0');
        const name = url.substring(url.lastIndexOf('/') + 1);

        return { type, size, name, url };
    };

    const fetchAttachment = async () => {
        const head = await fetchHead();

        setInfo(head);
    };

    useEffect(() => {
        fetchAttachment();
    }, [])

    if (info?.type.startsWith("image/")) {
        return <Img url={url} />
    } else if (info?.type.startsWith("video/")) {
        return <Vid url={url} />
    } else if (info) {
        return <File info={info} />
    } else {
        return null;
    }
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
                        message.attachments.map((url, i) => (
                            <Attachment
                                key={i}
                                url={url}
                            />
                        ))}
                </div>
            </div>
        </div>
    );
};

export default MessageComponent;
