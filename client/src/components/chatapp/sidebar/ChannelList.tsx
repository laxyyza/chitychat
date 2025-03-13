import { useEffect, useState } from 'react';
import { useApp, Action } from '../AppProvider';
import { Hub, Category, Channel } from '../Hub';

interface CategoryProp {
    category: Category;
}

interface ChannelProp {
    channel: Channel | undefined;
}

const ChannelComponent = ({ channel }: ChannelProp) => {
    if (channel === undefined) return null;

    const { app, dispatch } = useApp();

    useEffect(() => {
        const hub = app.hubs.get(app.currentHubID);
        dispatch({
            type: Action.SELECT_CHANNEL,
            payload: hub ? hub.channelIDs[hub.channelIDIndex] : -1
        });
    }, [app.currentHubID]);

    return (
        <div
            onClick={() => {
                const hub = app.hubs.get(app.currentHubID);
                if (hub) {
                    hub.channelIDIndex =
                        hub.channelIDs.findIndex((id) => id === channel.id) ||
                        0;
                }
                dispatch({ type: Action.SELECT_CHANNEL, payload: channel.id });
            }}
            className={`hover:bg-gray-400 pl-2 pr-2 m-1 rounded-xl select-none ${
                app.currentChannelID === channel.id ? 'bg-gray-500' : ''
            }`}
        >
            {channel.name}
        </div>
    );
};

const CategoryComponent = ({ category }: CategoryProp) => {
    const [open, setOpen] = useState(true);
    const { app } = useApp();

    return (
        <>
            <div
                onClick={() => setOpen(!open)}
                className="cursor-pointer select-none hover:bg-gray-400"
            >
                {open ? '▼' : '▶'} {category.name}
            </div>

            {open && (
                <ul>
                    {category.channelIDs.map((channelID) => (
                        <li key={channelID}>
                            <ChannelComponent
                                channel={app.textChannels.get(channelID)}
                            />
                        </li>
                    ))}
                </ul>
            )}
        </>
    );
};

const ChannelList = () => {
    const { app } = useApp();
    let hub: Hub | undefined = app.hubs.get(app.currentHubID);
    const [categories, setCategories] = useState<Category[] | null>(null);

    useEffect(() => {
        hub = app.hubs.get(app.currentHubID);
        setCategories(hub ? hub.categories : null);
    }, [app.currentChannelID, app.currentHubID]);

    return (
        <div className="flex-1 bg-gray-800 text-white">
            <div className="bg-gray-800 text-center shadow-md">
                Hub Settings
            </div>
            <ul>
                {categories &&
                    Array.from(categories.entries()).map(
                        ([id, category]) =>
                            hub && (
                                <li key={id}>
                                    <CategoryComponent
                                        category={category}
                                    ></CategoryComponent>
                                </li>
                            )
                    )}
            </ul>
        </div>
    );
};

export default ChannelList;
