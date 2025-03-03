import { ReactNode } from 'react';
import { IoAddCircleOutline } from 'react-icons/io5';
import { FaRegUser } from 'react-icons/fa';

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

interface ImgProp {
    url: string;
}

const HeaderBar = ({ name }: HeaderBarProp) => {
    return (
        <div className="bg-gray-800 text-center text-white shadow">{name}</div>
    );
};

const Img = ({ url }: ImgProp) => {
    return (
        <img
            className="rounded-4xl p-2 object-contain max-h-100 shrink"
            src={url}
        />
    );
};

const Message = ({ username, content }: MessageProp) => {
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
            <div className="m-1">
                {content} twasdiujqsdaoijd aoijdawoi djawodij awdoij
                daoidjaidjadoij daowijdawiodj doawijdawiojda oidjawod ijawdo
                iajda oijdao iwjdaoid jaodijawdoiawjd oaiwdjaoidjawodijawd
            </div>
            <div className="flex flex-wrap">
                {/* <Img url="https://www.whiskas.in/sites/g/files/fnmzdf2051/files/2024-10/cat-play.png"></Img>
                <Img url="https://i.pinimg.com/736x/2f/86/86/2f8686457ae5349508751f852111dace.jpg"></Img>
                <Img url="https://wallpapersok.com/images/hd/ultra-wide-4k-aesthetic-city-lights-fmsnpxlc1cpadvhm.jpg"></Img> */}
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
        <div className="flex flex-col flex-1 h-screen bg-gray-700">
            <HeaderBar name="Text Channel Name"></HeaderBar>
            <ChatWindow>{messages}</ChatWindow>
            <Input></Input>
        </div>
    );
};

export default MainContent;
