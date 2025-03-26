import SideBar from './components/chatapp/sidebar/SideBar';
import MainContent from './components/chatapp/MainContent';
import MemberList from './components/chatapp/MemberList';
import { Action, useApp } from './components/chatapp/AppProvider';
import { useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import websocketClient from './services/websocketClient';
import useWebsocket from './components/WebSocket';
import Group from './models/group';

function MainApp() {
    const { app, dispatch } = useApp();
    // const [test, setTest] = useState(0);
    const navigate = useNavigate();

    const { send } = useWebsocket((cmd, packet) => {
        switch (cmd) {
            case 'client_user_info': {
                dispatch({
                    type: Action.SET_LOGIN_USER,
                    payload: {
                        id: packet.user_id,
                        username: packet.username,
                        displayname: packet.displayname,
                        created_at: packet.create_at,
                        about_me: packet.bio,
                        pfp: ''
                    }
                });
                break;
            }
            case 'client_groups': {
                const groups: any[] = packet['groups'];
                groups.forEach((group) => {
                    dispatch({
                        type: Action.ADD_GROUP,
                        payload: new Group(
                            group['group_id'],
                            group['owner_id'],
                            group['name'],
                            '',
                            group['public']
                        )
                    });
                });
                break;
            }
            case 'get_user': {
                const users: any[] = packet['users'];
                users.forEach((user) => {
                    dispatch({
                        type: Action.ADD_USER,
                        payload: {
                            id: user['user_id'],
                            username: user['username'],
                            displayname: user['displayname'],
                            about_me: user['bio'],
                            pfp: '',
                            created_at: user['created_at']
                        }
                    });
                });
                break;
            }
            case 'group_msg': {
                dispatch({
                    type: Action.ADD_MSG,
                    payload: {
                        id: packet.msg_id,
                        channel_id: packet.group_id,
                        channel_type: 'group',
                        user_id: packet.user_id,
                        content: packet.content,
                        attachments: packet.attachments,
                        timestamp: packet.timestamp
                    }
                });
                break;
            }
        }
    });

    useEffect(() => {
        send({ cmd: 'client_user_info' });
        send({ cmd: 'client_groups' });
        fetch(window.location.origin + '/api/friends')
            .then((response) => response.json())
            .then((json) => {
                const friendIDs: number[] = json.friends;
                const donthaveIDs = friendIDs.filter(
                    (id) => !app.users.get(id)
                );

                if (donthaveIDs.length) {
                    send({ cmd: 'get_user', user_ids: donthaveIDs });
                }
                dispatch({ type: Action.ADD_FRIENDS, payload: json.friends });
            })
            .catch((error) => {
                console.error('fetch /api/friends:', error);
                if (
                    process.env.NODE_ENV === 'development' &&
                    app.friendIDs.length === 0
                ) {
                    send({ cmd: 'get_user', user_ids: [5, 6] });
                    dispatch({ type: Action.ADD_FRIENDS, payload: [5, 6] });
                }
            });

        if (process.env.NODE_ENV !== 'development') {
            websocketClient.onStateChange((state: string) => {
                if (state === 'error' || state === 'close') {
                    navigate('/login');
                }
            });
        }

        return () => websocketClient.onStateChange(undefined);
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
