import zmq
import numpy as np

import matplotlib.pyplot as plt

context = zmq.Context()

print("Connecting to server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

BUF_LEN = 10000

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)

lines = []

n_plots = 5
for i in range(n_plots):
    line, = ax.plot([])
    lines.append(line)

ax.set_xlim(0, BUF_LEN)
ax.set_ylim([0, 4096])

fig.canvas.draw()
plt.show(block=False)

i = 0
#for request in range(10):
while True:
    socket.send(b'Ready')
    print("Waiting for message")
    message = socket.recv()
    data = np.frombuffer(message, dtype=np.uint16)
    print(f"Received {data}")

    lines[i].set_data(np.arange(BUF_LEN), data)
    i = (i+1)%n_plots
    fig.canvas.draw()
    fig.canvas.flush_events()

