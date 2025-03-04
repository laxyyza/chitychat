import { ReactNode } from 'react';
import { IoAddCircleOutline } from 'react-icons/io5';
import { FaRegUser } from 'react-icons/fa';

interface MessageProp {
    username: string;
    content?: string;
    attachments?: string[];
}

interface ChatWindowProp {
    children: ReactNode[];
}

interface HeaderBarProp {
    name: string;
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

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-800 text-center text-white shadow">{name}</div>
    );
};

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

const Message = ({ username, content, attachments }: MessageProp) => {
    return (
        <div className="m-2 rounded-2xl p-2 hover:bg-gray-600 text-white flex-col max-w-full">
            <div className="flex">
                <FaRegUser
                    className="bg-red-600 m-1 mr-2 p-1 rounded-4xl"
                    size="42"
                ></FaRegUser>
                <div className="m-0 p-0 flex-1">
                    <div className="font-bold text-[18px]">
                        Display Name
                        <span className="m-6 text-[14px] font-normal">
                            Today at 10:30 am
                        </span>
                    </div>
                    <div className="text-[15px]">{username}</div>
                </div>
                {/* <div className="bg-black">yo</div> */}
            </div>
            <div className="m-1">{content}</div>
            <div className="flex flex-wrap">
                {attachments &&
                    attachments.map((url) => (
                        <Attachment
                            url={url}
                            type={url.endsWith('.mp4') ? 'video' : 'image'}
                        ></Attachment>
                    ))}
            </div>
        </div>
    );
};

const ChatWindow = ({ children }: ChatWindowProp) => {
    return <div className="flex-1 overflow-y-auto">{children}</div>;
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
        <Message
            username="username"
            content="Message content"
            attachments={[
                'https://wallpapersok.com/images/hd/ultra-wide-4k-aesthetic-city-lights-fmsnpxlc1cpadvhm.jpg'
            ]}
        />,
        <Message
            username="username"
            content="Message content"
            attachments={[
                'https://www.whiskas.in/sites/g/files/fnmzdf2051/files/2024-10/cat-play.png',
                'https://i.pinimg.com/736x/2f/86/86/2f8686457ae5349508751f852111dace.jpg'
            ]}
        />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message username="username" content="Message content" />,
        <Message
            username="username"
            content="Message content ccatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatcatat"
            attachments={[
                'https://www.whiskas.in/sites/g/files/fnmzdf2051/files/2024-10/cat-play.png'
            ]}
        />,
        <Message username="username" content="Message content" />,
        <Message
            username="username"
            content="Message content"
            attachments={['https://localhost:8080/video_av1.mp4']}
        />,
        <Message
            username="username"
            attachments={[
                'https://i.pinimg.com/736x/2f/86/86/2f8686457ae5349508751f852111dace.jpg'
            ]}
        />,
        <Message username="username" content="Message content" />
    ];

    return (
        <div className="flex flex-col flex-1 h-screen bg-gray-700">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow>{messages}</ChatWindow>
            <Input></Input>
        </div>
    );
};

export default MainContent;
