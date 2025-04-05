import { useEffect, useRef, useState } from 'react';
import { Action, DispatchAction, useApp } from './AppProvider';
import ChannelMessages from './ChannelMessages';
import { FaArrowDown } from 'react-icons/fa';
import Message from '../../models/message';
import useWebsocket from '../WebSocket';
import { DM, DMChat } from '../../models/dm';
import fetchData from '../../services/api';

const requestDMMessages = (dmChat: DMChat, dispatch: React.Dispatch<DispatchAction>) => {
    if (dmChat.chat instanceof DM == false) {
        return;
    }

    dmChat.requestMsgs = true;
    const limit = 20;

    fetchData(`/api/dms/${dmChat.chat.targetUserID}?limit=${limit}&offset=${dmChat.msgOffset}`)
        .then(data => {
            const messagesJson: any[] = data.messages;
            const messages: Message[] = [];

            messagesJson.forEach((msgJson) => {
                const msg: Message = {
                    id: msgJson.msg_id,
                    user_id: msgJson.user_id,
                    channel_id: msgJson.channel_id,
                    channel_type: "dm",
                    content: msgJson.content,
                    attachments: msgJson.attachments,
                    timestamp: msgJson.timestamp
                };

                messages.push(msg);
            })

            if (messages.length) {
                dispatch({ type: Action.ADD_DM_MSGS, payload: { dmID: dmChat.id, messages: messages } });
            }
        });
};

const getMessages = (): Message[] => {
    const { app, dispatch } = useApp();

    if (app.currentHubID !== -1) {
        const channel = app.textChannels.get(app.currentChannelID);
        if (channel) {
            return channel.messages;
        }
    } else if (app.currentDMID !== 'friends') {
        const dmchat = app.dm.get(app.currentDMID);
        if (dmchat) {
            const msgs = Array.from(dmchat.chat.messages.entries())
                .map(([_, msg]) => {
                    return msg;
                })
                .sort((a, b) => a.id - b.id);

            if (msgs.length === 0 && dmchat.requestMsgs === false) {
                requestDMMessages(dmchat, dispatch);
            }

            return msgs;
        }
    }

    return [];
};

const ChatWindow = () => {
    const { app, dispatch } = useApp();
    const appRef = useRef(app);
    const containerRef = useRef<HTMLDivElement | null>(null);
    const [isBottom, setIsBottom] = useState(true);
    const bottomRef = useRef<HTMLDivElement | null>(null);
    const messages = getMessages();
    const [isTop, setIsTop] = useState(false);
    const [oldScroll, setOldScroll] = useState(0);

    useEffect(() => {
        appRef.current = app;
    }, [app]);

    useWebsocket((cmd, packet) => {
        if (cmd === 'get_group_msgs') {
            const packet_msgs: any[] = packet.messages;
            const msgs: Message[] = packet_msgs.map((msg) => ({
                id: msg.msg_id,
                user_id: msg.user_id,
                channel_id: msg.group_id,
                channel_type: 'group',
                content: msg.content,
                attachments: msg.attachments,
                timestamp: msg.timestamp
            }));

            dispatch({
                type: Action.LOAD_GROUP_MSGS,
                payload: { group_id: packet.group_id, msgs: msgs }
            });
        }
    });

    function fetchMoreMessages(container: HTMLDivElement) {
        if (oldScroll === container.scrollHeight) {
            return;
        }

        setOldScroll(container.scrollHeight);

        // const group = appRef.current.groups.get(appRef.current.currentGroupID);
        // console.log('fetch message for ', group);

        // if (group && group.detailsLoaded) {
        //     send({
        //         cmd: 'get_group_msgs',
        //         group_id: group.id,
        //         limit: 10,
        //         offset: group.msgOffset
        //     });
        // }
    }

    const handleScroll = () => {
        const container = containerRef.current;
        if (!container) return;

        const threshold = 100;
        const bottom =
            container.scrollHeight -
            (container.scrollTop + container.clientHeight) <
            threshold;
        // const group = appRef.current.groups.get(appRef.current.currentGroupID);
        // if (group) {
        //     group.scrollTop = container.scrollTop;
        // }

        setIsTop(container.scrollTop === 0);
        if (container.scrollTop === 0) {
            console.log('Fetch messages!');
            fetchMoreMessages(container);
        }

        setIsBottom(bottom);
    };

    useEffect(() => {
        containerRef.current?.addEventListener('scroll', handleScroll);
        return () =>
            containerRef.current?.removeEventListener('scroll', handleScroll);
    }, []);

    useEffect(() => {
        if (isTop) {
            requestAnimationFrame(() => {
                if (containerRef.current) {
                    const newHeight = containerRef.current?.scrollHeight;
                    const diff = newHeight - oldScroll;

                    containerRef.current.scrollTop = diff;
                    setIsTop(false);
                }
            });
        } else if (isBottom) {
            bottomRef.current?.scrollIntoView({ behavior: 'instant' });
        }
    }, [messages]);

    // useEffect(() => {
    //     const group = appRef.current.groups.get(appRef.current.currentGroupID);
    //     if (group && group.scrollTop !== -1 && containerRef.current) {
    //         containerRef.current.scrollTop = group.scrollTop;
    //     }
    // }, [app.currentGroupID]);

    return (
        <>
            <div className="flex-1 overflow-y-auto" ref={containerRef}>
                <ChannelMessages messages={messages} />
                <div ref={bottomRef} />
            </div>
            {isBottom || (
                <div
                    className="bg-gray-800 border-1 border-black text-white self-center p-1 rounded-full hover:text-blue-400 active:bg-gray-700"
                    onClick={() =>
                        bottomRef.current?.scrollIntoView({
                            behavior: 'smooth'
                        })
                    }
                >
                    <FaArrowDown size="24" />
                </div>
            )}
        </>
    );
};

export default ChatWindow;
