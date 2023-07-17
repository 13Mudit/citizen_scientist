import sys
import time

import zmq
from multiprocessing import Process, Queue

import numpy as np

import matplotlib.pyplot as plt

from pyqtgraph.Qt import QtCore, QtWidgets
import pyqtgraph as pg


def get_data(queue: Queue):
    context = zmq.Context()

    print("Connecting to server...")
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://localhost:5555")


    socket.send(b'SOS' + b'\x00')
    print(socket.recv())

    while True:
        socket.send(b'1')
        # print("Waiting for message")
        message = socket.recv()
        data = np.frombuffer(message, dtype=np.uint16)
        
        print(f"Received\n{data}")

        queue.put(data)


class App(QtWidgets.QMainWindow):
    def __init__(self, parent=None):
        super(App, self).__init__(parent)

        #### Create Gui Elements ###########
        self.mainbox = QtWidgets.QWidget()
        self.setCentralWidget(self.mainbox)
        self.mainbox.setLayout(QtWidgets.QVBoxLayout())

        self.canvas = pg.GraphicsLayoutWidget()
        self.mainbox.layout().addWidget(self.canvas)

        self.label = QtWidgets.QLabel()
        self.mainbox.layout().addWidget(self.label)

        self.canvas.nextRow()
        #  line plot
        self.lineplot = self.canvas.addPlot()
        self.plots = []

        self.plots.append(self.lineplot.plot(pen='y'))
        self.plots.append(self.lineplot.plot(pen='r'))
        self.plots.append(self.lineplot.plot(pen='g'))
        self.plots.append(self.lineplot.plot(pen='b'))
        self.plots.append(self.lineplot.plot(pen='w'))

        self.current = 0


        #### Set Data  #####################

        self.x = np.linspace(0,50., num=100)

        self.counter = 0
        self.fps = 0.
        self.lastupdate = time.time()

        self.ydata = 0

        #### Create TCP Socket ############

        self.context = zmq.Context()

        print("Connecting to server...")
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect("tcp://localhost:5555")

        # FOR SOS Experiment
        self.socket.send(b'SOS' + b'\x00')
        print(self.socket.recv())

        #### Start  #####################
        self._update()

    def get_data(self):

        if self.ydata is not None:
            self.socket.send(b'1')
        # print("Waiting for message")

        try:
            message = self.socket.recv(flags=zmq.NOBLOCK)
        except zmq.Again:
            return None

        return np.frombuffer(message, dtype=np.uint16)

    def _update(self):

        self.ydata = self.get_data()

        if self.ydata is not None:
            self.plots[self.current].setData(self.ydata)
            self.current = (self.current + 1) % len(self.plots)


        now = time.time()
        dt = (now-self.lastupdate)
        if dt <= 0:
            dt = 0.000000000001
        fps2 = 1.0 / dt
        self.lastupdate = now
        self.fps = self.fps * 0.9 + fps2 * 0.1
        tx = 'Mean Frame Rate:  {fps:.3f} FPS'.format(fps=self.fps )
        self.label.setText(tx)
        QtCore.QTimer.singleShot(1, self._update)
        self.counter += 1

    def __del__(self):
        print("SOS App closed")
        self.socket.send(b'0')


if __name__ == '__main__':

    app = QtWidgets.QApplication(sys.argv)
    thisapp = App()
    thisapp.show()
    exit_code = app.exec()
    thisapp.__del__()
    sys.exit(exit_code)