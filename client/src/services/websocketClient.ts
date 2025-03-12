class WebSocketClient 
{
    private baseurl: string;
    private ws?: WebSocket;
    private listeners: Set<(event: MessageEvent<any>) => void>;
    private onStateChangeCallback?: (state: 'open' | 'close' | 'error') => void;
    public state: 'connecting' | 'open' | 'close' | 'error';

    constructor () 
    {
        this.baseurl = "wss://localhost:8080";
        this.listeners = new Set();
        this.state = 'close';
        this.onStateChangeCallback = undefined;
        this.connect();
    }

    connect(path: string = '') 
    {
        const url = this.baseurl + path;
        console.log("Connecting to ", url);
        this.ws = new WebSocket(url);
        this.state = 'connecting';

        this.ws.onopen = () => {
            this.state = 'open';
            console.log("OPEN");
            if (this.onStateChangeCallback)
                this.onStateChangeCallback(this.state);
        }

        this.ws.onmessage = (event) => {
            console.log(event.data);
            this.listeners.forEach((callback) => callback(event));
        }
        
        this.ws.onclose = () => {
            this.state = 'close';
            console.log("on close");
            if (this.onStateChangeCallback)
                this.onStateChangeCallback(this.state);
        }

        this.ws.onerror = () => {
            this.state = 'error';
            console.log("on error");
            if (this.onStateChangeCallback)
                this.onStateChangeCallback(this.state);
        }
    }

    send(msg: any) 
    {
        if (this.ws?.readyState === WebSocket.OPEN) {
            this.ws.send(JSON.stringify(msg));
        } else { 
            console.warn("Websocket not open. Not sending message.");
        }
    }

    subscribe(callback: (event: MessageEvent<any>) => void)
    {
        this.listeners.add(callback);
    }

    unsubscribe(callback: (event: MessageEvent<any>) => void) 
    {
        this.listeners.delete(callback);
    }

    onStateChange(callback?: (state: 'open' | 'close' | 'error') => void)
    {
        this.onStateChangeCallback = callback;
    }

    close()
    {
        this.ws?.close();
    }
}

const websocketClient = new WebSocketClient();

export default websocketClient;