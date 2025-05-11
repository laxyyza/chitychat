import { IoAddCircleOutline } from 'react-icons/io5';
import React, { useState, useRef } from 'react';
import { appGetFocusHub, useApp } from './AppProvider';
import { DM } from '../../models/dm';
import useWebsocket from '../WebSocket';
import Group from '../../models/group';
import Attachment from './Attachment';
// import useWebsocket from '../WebSocket';

interface Prop {
    placeholder?: string;
    attachments?: boolean;
}

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
    const [files, setFiles] = useState<File[]>([]);
    const inputFileRef = useRef<HTMLInputElement | null>(null);

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

    const doSendMessage = (fileUrls: string[] = []) => {
        if (app.focus.type === 'dm') {
            const dmchat = app.dm.get(app.focus.dmid);
            if (!dmchat) return;
            if (dmchat.chat instanceof DM) {
                send({
                    cmd: 'msg_user',
                    user_id: dmchat.chat.targetUserID,
                    content: message,
                    attachments: fileUrls
                })
            } else if (dmchat.chat instanceof Group) {
                send({
                    cmd: 'msg_group',
                    group_id: dmchat.chat.id,
                    content: message,
                    attachments: fileUrls
                })
            }
        } else if (app.focus.type === "hub") {
            const hub = appGetFocusHub(app);
            if (hub) {
                send({
                    cmd: 'msg_hub',
                    hub_id: hub.id,
                    channel_id: hub.selectedChannelID,
                    content: message,
                    attachments: fileUrls
                })
            }
        }

        setMessage(''); // Clear input after sending
        if (textareaRef.current) textareaRef.current.value = '';
        adjustHeight(); // Reset height
        setFiles([]);
    };

    const sendFile = async (file: File, callback: (url: string) => void) => {
        const formData = new FormData();
        formData.append('file', file);

        await fetch('https://localhost:8081/api/upload/' + file.name, {
            method: 'POST',
            body: formData,
        }).then(async (resp) => {
            const ret = await resp.json();
            callback("https://localhost:8081" + ret.endpoint);
        }).catch((e) => {
            console.log("E: ", e);
        });
    };

    const sendMessage = () => {
        if (!message.trim() && files.length === 0) return; // Prevent sending empty messages
        const fileUrls: string[] = [];

        files.forEach((file) => {
            sendFile(file, (url) => {
                fileUrls.push(url);
                if (fileUrls.length === files.length) {
                    doSendMessage(fileUrls);
                }
            });
        });

        if (files.length === 0) {
            doSendMessage();
        }
    };

    return (
        <>
            <div className='flex overflow-auto max-w-full'>
                {files.map((file) => (
                    <Attachment file={file} onDelete={() => {
                        setFiles(files.filter(f => f !== file));
                    }} />
                ))}
            </div>
            <div className="relative h-10" ref={divRef}>
                <div className="flex">
                    {attachments && (
                        <div className="">
                            <button className="">
                                <IoAddCircleOutline
                                    className="hover:bg-gray-600 active:bg-gray-500 rounded-l-2xl mr-2"
                                    size="40"
                                    onClick={() => setShowPopup(!showPopup)}
                                ></IoAddCircleOutline>
                            </button>
                            {showPopup && (
                                <div className='absolute overflow-hidden bottom-full rounded-xl bg-gray-900 border-1 border-gray-600 mb-2'>
                                    <button className='hover:bg-gray-800 p-3 rounded-xl text-nowrap' onClick={() => {
                                        inputFileRef.current?.click();
                                    }} >
                                        Upload File
                                    </button>
                                    <input ref={inputFileRef} className='absolute opacity-0' type='file' onChange={(e) => {
                                        if (e.target.files) {
                                            setFiles([...files, ...e.target.files]);
                                            setShowPopup(false);
                                        }
                                    }} />
                                </div>
                            )}
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
        </>
    );
};

export default Input;
