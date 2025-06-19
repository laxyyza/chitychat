import websocketClient from '../services/websocketClient';
import { useEffect } from 'react';

const useWebsocket = (onMessage?: (cmd: string, data: any) => void) => {
    useEffect(() => {
        if (!onMessage) return;

        websocketClient.subscribe(onMessage);

        return () => {
            websocketClient.unsubscribe(onMessage);
        };
    }, [onMessage]);

    return {
        send: websocketClient.send.bind(websocketClient)
    };
};

export default useWebsocket;
