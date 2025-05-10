import { useEffect, useState } from "react";
import { useParams } from "react-router-dom";
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
};

interface JoinProps {
    data: InviteData;
};

const JoinPage = ({ data }: JoinProps) => {
    const { app } = useApp();

    return (
        <>
            <div className="text-gray-300 m-5">
                <span className="font-bold">{data?.displayname}</span> ({data?.username}) invited you to
            </div>
            <div className="text-3xl">{data?.hub_name}</div>
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
            <button className="bg-blue-500 hover:bg-blue-400 p-3 text-xl rounded-xl font-bold mt-auto">
                Join
            </button>
        </>
    );
};

const InvitePage = () => {
    const { code } = useParams();
    const [data, setData] = useState<InviteData | null>(null);
    const [error, setError] = useState('');
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
                    setError(err.error || (err.status));
                });
            })
        .catch(() => {
            setError("Not logged in!");
        });
    }, []);

    const renderPage = () => {
        if (error) {
            return (
                <>
                    <div>ERROR:</div>
                    <div>{error}</div>
                </>
            );
        }
        else if (data !== null) {
            return <JoinPage data={data} />
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
