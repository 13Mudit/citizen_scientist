import zmq
import numpy as np

# import matplotlib.pyplot as plt

context = zmq.Context()

print("Connecting to server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")


socket.send(b'RLC' + b'\x00')
print(socket.recv())

while True:
    socket.send(b'1')
    # print("Waiting for message")
    message = socket.recv()
    data = np.frombuffer(message, dtype=np.uint16)
    
    print(f"Received\n{data}")