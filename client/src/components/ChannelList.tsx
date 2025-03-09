import { useEffect, useState } from 'react';
import { useApp, Action } from './AppProvider';
import { Hub, Category, Channel } from './Hub';

interface CategoryProp {
    category: Category;
    hub: Hub;
}

interface ChannelProp {
    channel: Channel | undefined;
}

const ChannelComponent = ({ channel }: ChannelProp) => {
    if (channel === undefined) return null;

    const { app, dispatch } = useApp();

    return (
        <li
            onClick={() =>
                dispatch({ type: Action.SELECT_CHANNEL, payload: channel.id })
            }
            className={`hover:bg-gray-400 pl-2 pr-2 m-1 rounded-xl select-none ${
                app.selectedChannelID === channel.id ? 'bg-gray-500' : ''
            }`}
        >
            {channel.name}
        </li>
    );
};

const CategoryComponent = ({ category, hub }: CategoryProp) => {
    const [open, setOpen] = useState(true);

    return (
        <div>
            <div
                onClick={() => setOpen(!open)}
                className="cursor-pointer select-none hover:bg-gray-400"
            >
                {open ? '▼' : '▶'} {category.name}
            </div>

            {open && (
                <ul>
                    {category.channelIDs.map((channelID) => (
                        <ChannelComponent
                            channel={hub.channels.get(channelID)}
                        />
                    ))}
                </ul>
            )}
        </div>
    );
};

const ChannelList = () => {
    const { app } = useApp();
    let hub: Hub | null = app.hubs[app.hubIndex];
    const [categories, setCategories] = useState<Map<number, Category> | null>(
        null
    );

    useEffect(() => {
        hub = app.hubs[app.hubIndex];
        setCategories(hub ? hub.categories : null);
    }, [app.hubIndex]);

    return (
        <div className="flex-1 bg-gray-800 text-white">
            <div className="bg-gray-800 text-center shadow-md">
                Hub Settings
            </div>
            {categories &&
                Array.from(categories.entries()).map(
                    ([id, category]) =>
                        hub && (
                            <CategoryComponent
                                category={category}
                                hub={hub}
                            ></CategoryComponent>
                        )
                )}
        </div>
    );
};

export default ChannelList;
