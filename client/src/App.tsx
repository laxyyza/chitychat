import SideBar from './components/SideBar';
import MainContent from './components/MainContent';
import MemberList from './components/MemberList';
import { AppCtx } from './components/AppProvider';
import { useContext } from 'react';
import { Hub } from './components/Hub';

function MainApp() {
    const app = useContext(AppCtx);

    app.hubs = [
        new Hub(
            1,
            1,
            "McDonald's",
            'https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcR-Xzk84KtRUPnOh9AKiA_kJgqqnYocZtZJ7GCw6OzmuxPuXOIozxBwrR5fHmzqbXzeHQc&usqp=CAU'
        ),
        new Hub(1, 1, "Laxyy's Hub")
    ];
    app.login_user = {
        id: 1,
        username: 'username',
        displayname: 'Display Name',
        pfp: 'https://www.oola.com/wp-content/uploads/2022/07/communityIcon_x4lqmqzu1hi81.jpeg',
        created_at: '15 January 2025, 12:30 PM',
        about_me: 'About me'
    };

    return (
        <div className="flex">
            <SideBar></SideBar>
            <MainContent></MainContent>
            <MemberList></MemberList>
        </div>
    );
}

export default MainApp;
