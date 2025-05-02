import { IoAddCircleOutline } from 'react-icons/io5';
import React, { useState, useRef } from 'react';
import { useApp } from './AppProvider';
import { DM } from '../../models/dm';
import useWebsocket from '../WebSocket';
import Group from '../../models/group';
// import useWebsocket from '../WebSocket';

interface Prop {
    placeholder?: string;
    attachments?: boolean;
}

const Popup = () => {
    return (
        <div className="absolute bg-gray-950 text-white bottom-12 p-2 rounded-xl w-35 select-none border-black border-1">
            <div className="hover:bg-blue-600 p-2 rounded-xl active:bg-blue-500">
                Upload a file
            </div>
            <div className="hover:bg-blue-600 p-2 rounded-xl active:bg-blue-500">
                Create a poll
            </div>
        </div>
    );
};

const Input = ({
    placeholder = 'Type a message...',
    attachments = true
}: Prop) => {
    const [message, setMessage] = useState('');
    const textareaRef = useRef<HTMLTextAreaElement | null>(null);
    const divRef = useRef<HTMLDivElement | null>(null);
    const [showPopup, setShowPopup] = useState(false);
    const { app } = useApp();
    // const [counter, setCounter] = useState(4);

    const { send } = useWebsocket();

    if (app.focus.type === "friends") return null;

    const adjustHeight = () => {
        const textarea = textareaRef.current;
        const div = divRef.current;
        if (textarea) {
            textarea.style.height = 'auto'; // Reset height
            textarea.style.height = `${textarea.scrollHeight}px`; // Set new height
        }
        if (div) {
            div.style.height = 'auto';
            div.style.height = `${div.scrollHeight}px`; // Set new height
        }
    };

    const handleChange = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
        setMessage(event.target.value);
        adjustHeight();
        setShowPopup(false);
    };

    const handleKeyDown = (event: React.KeyboardEvent<HTMLTextAreaElement>) => {
        if (event.key === 'Enter' && !event.shiftKey) {
            event.preventDefault(); // Prevents new line from being added
            sendMessage();
        }
    };

    const sendMessage = () => {
        if (!message.trim()) return; // Prevent sending empty messages

        if (app.focus.type === 'dm') {
            const dmchat = app.dm.get(app.focus.dmid);
            if (!dmchat) return;
            if (dmchat.chat instanceof DM) {
                send({
                    cmd: 'msg_user',
                    user_id: dmchat.chat.targetUserID,
                    content: message,
                    attachments: []
                })
            } else if (dmchat.chat instanceof Group) {
                send({
                    cmd: 'msg_group',
                    group_id: dmchat.chat.id,
                    content: message,
                    attachments: []
                })
            }
        } else if (app.focus.type === "hub") {
            console.log("IMPLEMENT 'msg_hub'!")
        }

        // const channel_type = app.currentHubID === -1 ? 'group' : 'hub';
        // const channel_id =
        //     channel_type === 'hub' ? app.currentChannelID : app.currentDMID;

        // if (channel_type === 'hub') {
        //     setCounter(counter + 1);
        //     dispatch({
        //         type: Action.ADD_MSG,
        //         payload: {
        //             id: counter,
        //             user_id: app.login_user.id,
        //             channel_id: channel_id,
        //             channel_type: channel_type,
        //             content: message,
        //             attachments: [],
        //             timestamp: '<timestamp>'
        //         }
        //     });
        // } else {
        //     send({
        //         cmd: 'group_msg',
        //         group_id: channel_id,
        //         content: message,
        //         attachments: []
        //     });
        // }

        console.log('Sent:', message);
        setMessage(''); // Clear input after sending
        if (textareaRef.current) textareaRef.current.value = '';
        adjustHeight(); // Reset height
    };

    return (
        <div className="relative h-10" ref={divRef}>
            <div className="flex">
                {attachments && (
                    <div className="">
                        <div className="relative">
                            <IoAddCircleOutline
                                className="hover:bg-gray-600 active:bg-gray-500 rounded-l-2xl mr-2"
                                size="40"
                                onClick={() => setShowPopup(!showPopup)}
                            ></IoAddCircleOutline>
                            {showPopup && <Popup></Popup>}
                        </div>
                    </div>
                )}
                <div className="flex-1">
                    <textarea
                        ref={textareaRef}
                        value={message}
                        onChange={handleChange}
                        onKeyDown={handleKeyDown}
                        placeholder={placeholder}
                        className="w-full h-full outline-0 text-2xl resize-none"
                        rows={1}
                    ></textarea>
                </div>
            </div>
        </div>
    );
};

export default Input;
