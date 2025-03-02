import { useState } from 'react';

interface ListProp {
    title: string;
    items: string[];
}

interface ChannelProp {
    name: string;
}

const Channel = ({ name }: ChannelProp) => {
    return (
        <li
            className="hover:bg-gray-400 pl-2 pr-2 m-1 rounded-xl select-none"
            key={name}
        >
            {name}
        </li>
    );
};

const List = ({ title, items }: ListProp) => {
    const [open, setOpen] = useState(true);

    return (
        <div>
            <div
                onClick={() => setOpen(!open)}
                className="cursor-pointer select-none hover:bg-gray-400"
            >
                {open ? '▼' : '▶'} {title}
            </div>

            {open && (
                <ul>
                    {items.map((item) => (
                        <Channel name={item} />
                    ))}
                </ul>
            )}
        </div>
    );
};

const ChannelList = () => {
    const items1 = ['General', 'Clips'];
    const items2 = ['General', 'Meeting'];

    return (
        <div className="channels">
            <div className="bg-green-950 text-center">Hub Settings</div>
            <Channel name="Filesystem"></Channel>
            <List title="Text Channels" items={items1}></List>
            <List title="Voice Channels" items={items2}></List>
        </div>
    );
};

export default ChannelList;
