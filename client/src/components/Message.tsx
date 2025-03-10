import UserIcon from './UserIcon';
import { Message } from './Hub';
import { useApp } from './AppProvider';

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

const MessageComponent = ({message}: Prop) => {
    const {app} = useApp();
    const user = app.users.get(message.user_id);

    if (!user) return null;

    return (
        <div className="relative m-2 rounded-2xl p-2 hover:bg-gray-600 text-white flex-col max-w-full">
            <div className="flex">
                <UserIcon user={user}></UserIcon>
                <div className="ml-2 m-0 p-0 flex-1">
                    <div className="font-bold text-[18px]">
                        {user.displayname}
                        <span className="m-6 text-[14px] font-normal">
                            Today at 10:30 am
                        </span>
                    </div>
                    <div className="text-[15px]">{user.username}</div>
                </div>
                {/* <div className="bg-black">yo</div> */}
            </div>
            <div className="m-1">{message.content}</div>
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
    );
};

export default MessageComponent;
