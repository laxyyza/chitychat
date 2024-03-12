#!/usr/bin/env python3

import websocket

class Bot:
    def __init__(self) -> None:
        self.ws = websocket.WebSocketApp("wss://chitychat.com:8080", 
            on_open=self.on_open,
            on_message=self.on_message,
            on_close=self.on_close
        )
    
    def on_close(self, close_state_code, close_msg):
        print("Close " + close_state_code + " msg " + close_msg)
    
    def on_message(self, ws, msg):
        print("msg:", msg)

    def on_open(self, ws):
        print("Connected!")
    
    def run(self):
        self.ws.run_forever()

if __name__ == '__main__':
    bot = Bot()
    bot.run()
