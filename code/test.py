import random
import tkinter as Tk
from itertools import count

import numpy as np

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


import zmq


context = zmq.Context()

print("Connecting to server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")


socket.send(b'RLC')
print(str(socket.recv()))

plt.style.use('fivethirtyeight')


def animate(i):
    socket.send(b'1')
    print("Waiting for message")
    message = socket.recv()
    data = np.frombuffer(message, dtype=np.uint16)
    data = data[7:]
    x_vals = np.array(len(data))

    # Get all axes of figure
    ax = plt.gcf().get_axes()
    # Clear current data
    ax.cla()
    # Plot new data
    ax.plot(x_vals, data)


# GUI
root = Tk.Tk()
label = Tk.Label(root, text="Realtime Animated Graphs").grid(column=0, row=0)

# graph 1
canvas = FigureCanvasTkAgg(plt.gcf(), master=root)
canvas.get_tk_widget().grid(column=0, row=1)
# Create two subplots in row 1 and column 1, 2
plt.gcf().subplots(1, 1)
ani = FuncAnimation(plt.gcf(), animate, interval=100, blit=False)

Tk.mainloop()