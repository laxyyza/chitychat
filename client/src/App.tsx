import SideBar from './components/chatapp/sidebar/SideBar';
import MainContent from './components/chatapp/MainContent';
import MemberList from './components/chatapp/MemberList';
import { Action, App, DispatchAction, useApp } from './components/chatapp/AppProvider';
import { useEffect } from 'react';
// import { useNavigate } from 'react-router-dom';
import websocketClient from './services/websocketClient';
import useWebsocket from './components/WebSocket';
import handleWebsocketMessage from './hooks/useWebsocket';
import fetchData from './services/api';
import { HubBasicData } from './models/hub';

const loadAppData = (app: App, send: (msg: any) => void, dispatch: React.Dispatch<DispatchAction>) => {
    send({ cmd: "client_user_info" });
    send({ cmd: 'client_groups' });

    fetchData('/api/friends')
        .then(json => {
            const friendIDs: number[] = json.friends;
            const donthaveIDs = friendIDs.filter(
                (id) => !app.users.get(id)
            );

            if (donthaveIDs.length) {
                send({ cmd: 'get_user', user_ids: donthaveIDs });
            }
            dispatch({ type: Action.ADD_FRIENDS, payload: json.friends });
        });

    fetchData('/api/friends/requests')
        .then((json) => {
            const userIDs: number[] = json.user_ids;
            const donthaveIDs = userIDs.filter((id) => !app.users.get(id));

            if (donthaveIDs.length) {
                send({ cmd: 'get_user', user_ids: donthaveIDs });
            }
            dispatch({
                type: Action.ADD_FRIEND_REQUESTS,
                payload: json.user_ids
            });
        });

    fetchData('/api/friends/requests/outgoing')
        .then((json) => {
            const userIDs: number[] = json.user_ids;
            const donthaveIDs = userIDs.filter((id) => !app.users.get(id));

            if (donthaveIDs.length) {
                send({ cmd: 'get_user', user_ids: donthaveIDs });
            }
            dispatch({
                type: Action.ADD_PENDING_FRIEND_REQUESTS,
                payload: json.user_ids
            });
        });

    fetchData('/api/hubs')
        .then((json) => {
            const basicData: HubBasicData[] = json.hubs;

            dispatch({
                type: Action.ADD_BASIC_HUBS,
                payload: basicData
            })
        })

    return () => websocketClient.onStateChange(undefined);
};

function MainApp() {
    const { app, dispatch } = useApp();
    // const [test, setTest] = useState(0);
    // const navigate = useNavigate();

    const { send } = useWebsocket((cmd, packet) => {
        handleWebsocketMessage(cmd, packet, app, dispatch);
    });

    useEffect(() => {
        loadAppData(app, send, dispatch);
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
