import { IoAddCircleOutline } from 'react-icons/io5';
import React, { useState, useRef } from 'react';

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

const Input = () => {
    const [message, setMessage] = useState('');
    const textareaRef = useRef<HTMLTextAreaElement | null>(null);
    const divRef = useRef<HTMLDivElement | null>(null);
    const [showPopup, setShowPopup] = useState(false);

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
        console.log('Change: ', event.target.value);
        setMessage(event.target.value);
        adjustHeight();
    };

    const handleKeyDown = (event: React.KeyboardEvent<HTMLTextAreaElement>) => {
        if (event.key === 'Enter' && !event.shiftKey) {
            event.preventDefault(); // Prevents new line from being added
            sendMessage();
        }
    };

    const sendMessage = () => {
        if (!message.trim()) return; // Prevent sending empty messages
        console.log('Sent:', message);
        setMessage(''); // Clear input after sending
        textareaRef.current.value = '';
        adjustHeight(); // Reset height
    };

    return (
        <div className="relative h-10 m-2 flex max-h-100" ref={divRef}>
            <div className="bg-gray-900 text-white rounded-2xl flex flex-1">
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
                <div className="overflow-auto h-full w-full">
                    <textarea
                        ref={textareaRef}
                        value={message}
                        onChange={handleChange}
                        onKeyDown={handleKeyDown}
                        placeholder="Type a message..."
                        className="w-full h-full outline-0 text-2xl resize-none"
                        rows={1}
                    ></textarea>
                </div>
            </div>
        </div>
    );
};

export default Input;
