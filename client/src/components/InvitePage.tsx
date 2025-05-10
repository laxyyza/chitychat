import { useEffect, useState } from "react";
import { useNavigate, useParams } from "react-router-dom";
import fetchData from "../services/api";
import { useApp } from "./chatapp/AppProvider";
import UserProfile from "./chatapp/sidebar/UserProfile";
import useWebsocket from "./WebSocket";
import handleWebsocketMessage from "../hooks/useWebsocket";

interface InviteData {
    hub_name: string;
    expires_at: string | null;
    username: string;
    displayname: string;
    uses: number;
    members_count: number;
    already_joined: boolean;
};

interface JoinProps {
    data: InviteData;
    code: string;
    setErrorMsg: (msg: string) => void;
};

const JoinPage = ({ data, code, setErrorMsg }: JoinProps) => {
    const { app } = useApp();
    const navigator = useNavigate();

    const joined = data.already_joined;

    const join = () => {
        if (joined) return;

        fetchData(`/api/hubs/invites/${code}`, 'POST')
            .then(() => {
                // TODO: navigator(`/app/hubs/${resp.hub_id}`);
                navigator('/app');
            })
            .catch((err) => {
                setErrorMsg(err);
            });
    };

    return (
        <>
            <div className="text-gray-300 m-5">
                <span className="font-bold">{data?.displayname}</span> ({data?.username}) invited you to
            </div>
            <div className="text-4xl">{data?.hub_name}</div>
            <div>
                <span className="font-bold">
                    {data?.members_count}
                </span> members
            </div>
            <div className="m-3 bg-gray-950 rounded-xl p-3">
                <div>
                    Joining as
                </div>
                <UserProfile user={app.login_user} showSettings={false} />
            </div>
            <button 
                className={`${joined ? "bg-gray-700 text-gray-400" : "bg-blue-500 hover:bg-blue-400 active:bg-blue-300"} p-3 text-xl rounded-xl font-bold mt-auto`}
                onClick={join}
            >
                {joined ? "Joined" : "Join"}
            </button>
        </>
    );
};

const InvitePage = () => {
    const { code } = useParams();
    const [data, setData] = useState<InviteData | null>(null);
    const [errorMsg, setErrorMsg] = useState('');
    const {app, dispatch} = useApp();

    const { send } = useWebsocket((cmd, packet) => {
        handleWebsocketMessage(cmd, packet, app, dispatch);
    });

    useEffect(() => {
        send({ cmd: "client_user_info" });
        fetchData('/api/auth/remember').then(() => {
            fetchData(`/api/hubs/invites/${code}`)
                .then((resp) => {
                    setData(resp);
                })
                .catch((err) => {
                    if (err.error) {
                        setErrorMsg(err.error);
                    } else {
                        setErrorMsg(err);
                    }
                });
            })
        .catch(() => {
            setErrorMsg("Not logged in!");
        });
    }, []);
    
    const renderPage = () => {
        if (errorMsg) {
            return (
                <div className="m-auto">
                    <div className="text-3xl">{errorMsg}</div>
                </div>
            );
        }
        else if (data !== null && code) {
            return <JoinPage data={data} code={code} setErrorMsg={setErrorMsg} />
        } else {
            return (
                <div className="flex items-center justify-center h-screen">
                    <div className="text-center space-y-4">
                        <div className="animate-spin rounded-full h-12 w-12 border-4 border-t-transparent border-b-transparent border-blue-500 mx-auto" />
                        <p className="text-gray-600 text-lg">Loading invite details...</p>
                    </div>
                </div>
            );
        }
    };

    return (
        <div className="bg-gray-800 flex items-center justify-center w-screen h-screen text-white">
            <div className="bg-gray-900 w-125 h-120 rounded-xl flex flex-col text-center p-10">
                {renderPage()}
            </div>
        </div>
    );
};

export default InvitePage;
