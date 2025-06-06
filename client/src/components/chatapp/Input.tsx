import { IoAddCircleOutline } from 'react-icons/io5';
import React, { useState, useRef } from 'react';
import { DM, DMChat } from '../../models/dm';
import useWebsocket from '../WebSocket';
import Attachment from './Attachment';
import { Hub } from '../../models/hub';
import User from './User';
// import useWebsocket from '../WebSocket';

interface Props {
    placeholder: string;
    target: Hub | DMChat | User;
    attachments?: boolean;
    afterMessageSent?: () => void;
}

const Input = ({
    placeholder,
    target,
    attachments = false,
    afterMessageSent
}: Props) => {
    const [message, setMessage] = useState('');
    const inputFileRef = useRef<HTMLInputElement | null>(null);
    const textareaRef = useRef<HTMLTextAreaElement | null>(null);
    const [showPopup, setShowPopup] = useState(false);
    const [files, setFiles] = useState<File[]>([]);

    const { send } = useWebsocket();

    const adjustHeight = () => {
        const textarea = textareaRef.current;
        if (textarea) {
            textarea.style.height = 'auto'; // Reset height
            textarea.style.height = `${textarea.scrollHeight}px`; // Set new height
        }
    };

    const handleChange = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
        setMessage(event.target.value);
        adjustHeight();
        setShowPopup(false);
    };

    const sendFile = async (file: File) => {
        const formData = new FormData();
        formData.append('file', file);

        // TODO: DONT HARD CODE URL!
        return await fetch('https://localhost:8081/api/upload/' + file.name, {
            method: 'POST',
            body: formData,
        }).then(async (resp) => {
            const ret = await resp.json();
            return `https://localhost:8081${ret.endpoint}`;
        });
    };

    const doSendMessage = (fileUrls: string[]) => {
        if (target instanceof DMChat) {
            if (target.chat instanceof DM) {
                send({
                    cmd: 'msg_user',
                    user_id: target.chat.targetUserID,
                    content: message,
                    attachments: fileUrls
                });
            } else {
                send({
                    cmd: 'msg_group',
                    group_id: target.chat.id,
                    content: message,
                    attachments: fileUrls
                });
            }
        } else if (target instanceof Hub) {
            send({
                cmd: 'msg_hub',
                hub_id: target.id,
                channel_id: target.selectedChannelID,
                content: message,
                attachments: fileUrls
            });
        }

        setMessage(''); // Clear input after sending
        if (textareaRef.current) textareaRef.current.value = '';
        adjustHeight(); // Reset height
        setFiles([]);
    };

    const sendMessage = async () => {
        if (!message.trim() && files.length === 0) return;
        let attachUrls: string[] = [];

        if (files.length) {
            attachUrls = await Promise.all(
                files.map(file => sendFile(file))
            );
        }

        doSendMessage(attachUrls);

        if (afterMessageSent) {
            afterMessageSent();
        }
    };

    const handleKeyDown = (event: React.KeyboardEvent<HTMLTextAreaElement>) => {
        if (event.key === 'Enter' && !event.shiftKey) {
            event.preventDefault(); // Prevents new line from being added
            sendMessage();
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
            <div className="flex flex-1 items-center relative">
                {attachments && (
                    <>
                        <button className='self-start'>
                            <IoAddCircleOutline
                                className="hover:bg-gray-600 active:bg-gray-500 rounded-l-2xl mr-2"
                                size={"40"}
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
                    </>
                )}
                <textarea
                    ref={textareaRef}
                    value={message}
                    onChange={handleChange}
                    onKeyDown={handleKeyDown}
                    placeholder={placeholder}
                    className="w-full h-full text-2xl outline-0 resize-none"
                    rows={1}
                />
            </div>
        </>
    );
};

export default Input;
