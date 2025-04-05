import { useEffect, useRef, useState } from 'react';
import { Action, useApp } from './AppProvider';
import useWebsocket from '../WebSocket';
import fetchData from '../../services/api';

interface Prop {
    onClose: () => void;
}

const AddFriend = ({ onClose }: Prop) => {
    const { app, dispatch } = useApp();
    const [username, setUsername] = useState('');
    const inputRef = useRef<HTMLInputElement | null>(null);
    const [errorMsg, setErrorMsg] = useState('');

    const { send } = useWebsocket();

    const postRequest = () => {
        fetchData('/api/friend-request', 'POST', { username: username })
            .then(data => {
                const userID: number = data.user_id
                if (!app.users.get(userID)) {
                    send({ cmd: 'get_user', user_ids: [userID] });
                }

                dispatch({
                    type: Action.ADD_PENDING_FRIEND_REQUESTS,
                    payload: [userID]
                });
                setErrorMsg('');
                onClose();
            })
            .catch(error => setErrorMsg('ERROR: ' + error));
    };

    useEffect(() => {
        inputRef.current?.focus();
    }, []);

    // Close when clicking outside
    useEffect(() => {
        const handleKeyEvent = (event: globalThis.KeyboardEvent) => {
            if (event.key == 'Escape') if (onClose) onClose();
        };

        document.addEventListener('keydown', handleKeyEvent);
        return () => {
            document.removeEventListener('keydown', handleKeyEvent);
        };
    }, []);

    return (
        <div
            className="absolute flex justify-center items-center w-full h-full z-1000 backdrop-blur-xs text-white"
            onClick={() => onClose()}
            onKeyUp={(e) => {
                console.log('key ', e.key);
                if (e.key == 'Escape') onClose();
            }}
        >
            <div
                className="bg-gray-800 w-150 p-2 rounded-xl border-1 border-black"
                onClick={(e) => e.stopPropagation()}
            >
                {errorMsg && (
                    <div className="bg-red-500 mb-2 rounded-xs p-1 font-bold">
                        <span>{errorMsg}</span>
                    </div>
                )}
                {app.friendIDs.length === 0 && (
                    <div className="mb-2 text-xl">
                        No friends yet? Send a request and start a conversation!
                    </div>
                )}
                <form
                    onSubmit={(e) => {
                        e.preventDefault();
                        postRequest();
                    }}
                    className="flex"
                >
                    <input
                        className="flex-1 bg-gray-900 rounded-xl mr-1 h-8 outline-0 pl-2 focus"
                        type="text"
                        placeholder="Enter your friend's username"
                        value={username}
                        required
                        ref={inputRef}
                        onChange={(e) => {
                            setUsername(e.target.value);
                        }}
                    />
                    <button
                        type="submit"
                        className="bg-blue-600 hover:bg-blue-500 active:bg-blue-400 p-1 rounded-xl"
                    >
                        Send Friend Request
                    </button>
                </form>
            </div>
        </div>
    );
};

export default AddFriend;
