import SideBar from './components/SideBar';
import MainContent from './components/MainContent';
import MemberList from './components/MemberList';
import { useApp } from './components/AppProvider';
import { Hub, ChannelType } from './components/Hub';
import { useEffect, useState } from 'react';

function MainApp() {
    const { app } = useApp();
    const [test, setTest] = useState(0);

    useEffect(() => {
        app.hubs.set(
            1,
            new Hub(
                1,
                1,
                "McDonald's",
                'https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcR-Xzk84KtRUPnOh9AKiA_kJgqqnYocZtZJ7GCw6OzmuxPuXOIozxBwrR5fHmzqbXzeHQc&usqp=CAU',
                [1, 2]
            )
        );
        app.hubs.set(2, new Hub(2, 1, "Laxyy's Hub", null, [3]));

        app.login_user = {
            id: 1,
            username: 'username',
            displayname: 'Display Name',
            pfp: 'https://www.oola.com/wp-content/uploads/2022/07/communityIcon_x4lqmqzu1hi81.jpeg',
            created_at: '15 January 2025, 12:30 PM',
            about_me: 'About me'
        };
        app.users.set(app.login_user.id, app.login_user);
        app.users.set(2, {
            id: 2,
            username: 'laxyyza',
            displayname: 'Laxyy',
            pfp: 'https://i.pinimg.com/236x/1a/ae/bc/1aaebcf79c6ef766603c655b3bef104f.jpg',
            created_at: '?',
            about_me: ''
        });

        app.hubs.get(1)?.memberIDs.push(2);
        app.textChannels.set(1, {
            type: ChannelType.TEXT,
            id: 1,
            name: 'General',
            hub_id: 1,
            messages: [
                {
                    id: 1,
                    user_id: 1,
                    channel_id: 2,
                    content: 'Test message from TS',
                    attachments: []
                }
            ]
        });
        app.textChannels.set(2, {
            type: ChannelType.TEXT,
            id: 2,
            name: 'Vent',
            hub_id: 1,
            messages: [
                {
                    id: 3,
                    user_id: 1,
                    channel_id: 2,
                    content: 'Test message from TS VENT',
                    attachments: []
                }
            ]
        });
        app.textChannels.set(3, {
            type: ChannelType.TEXT,
            id: 3,
            name: 'General',
            hub_id: 2,
            messages: []
        });

        app.hubs.get(1)?.categories.push({
            id: 1,
            name: 'Text Channels',
            channelIDs: [1, 2]
        });
        app.hubs.get(2)?.categories.push({
            id: 2,
            name: 'Text Channels',
            channelIDs: [3]
        });

        setTest(test + 1);
    }, []);

    return (
        <div className="flex">
            <SideBar></SideBar>
            <MainContent></MainContent>
            <MemberList></MemberList>
        </div>
    );
}

export default MainApp;
