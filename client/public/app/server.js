import {App} from './app.js';

export class Server
{
    constructor(app)
    {
        this.app = app;
        // this.open_event = app.server_open;
        // this.msg_event = app.server_msg;
        // this.error_event = app.server_error;
        // this.close_event = app.server_close;
        this.connected = false;
        this.pending_packets = [];
        this.init_socket();
    }

    init_socket()
    {
        this.socket = new WebSocket("wss://" + window.location.hostname + ":" + window.location.port);

        this.socket.addEventListener('open', (event) => {
            this.connected = true;

            for (let i = 0; i < this.pending_packets.length; i++)
            {
                const packet = this.pending_packets[i];
                this.socket.send(packet);
            }
            this.pending_packets = [];

            this.app.server_open(event);
            // if (this.open_event)
            //     this.open_event(event);
        });

        this.socket.addEventListener('message', (event) => {
            const packet = JSON.parse(event.data);
            this.app.server_msg(packet);
            // this.msg_event(payload);
        });

        this.socket.addEventListener('error', (event) => {
            console.error("Server WS ERROR:", event.reason);
            this.app.server_error(event);
        });

        this.socket.addEventListener('close', (event) => {
            this.connected = false;
            this.app.server_close(event);

            this.init_socket();
        });
    }
    
    ws_send(object)
    {
        const packet = JSON.stringify(object);
        if (this.connected)
        {
            this.socket.send(packet);
        }
        else
        {
            this.pending_packets.push(packet);
            this.init_socket();
        }
    }
}
