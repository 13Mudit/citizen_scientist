import sys
import time

import zmq

import numpy as np

import matplotlib.pyplot as plt

from pyqtgraph.Qt import QtCore, QtWidgets
import pyqtgraph as pg


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
        self.otherplot = self.canvas.addPlot()

        self.otherplot.setYRange(-5, 65)
        # self.otherplot.setXRange(0, 4)

        self.plot = self.otherplot.plot(symbol='o')
        self.plot.setSymbolSize(5)

        #### Set Data  #####################

        self.counter = 0
        self.fps = 0.
        self.lastupdate = time.time()

        #### Create TCP Socket ############

        self.context = zmq.Context()

        print("Connecting to server...")
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect("tcp://localhost:5555")

        # FOR RLC Experiment
        self.socket.send(b'DIODE' + b'\x00')
        print(self.socket.recv())

        #### Start  #####################
        self._update()

    def get_data(self):
        self.socket.send(b'1')
        # print("Waiting for message")
        message = self.socket.recv()
        data = np.frombuffer(message, dtype=np.uint16)
        

        voltage = data[:len(data)//2]
        current = data[len(data)//2:]

        voltage = np.average(voltage.reshape(-1, 8), axis=1)
        current = np.average(current.reshape(-1, 8), axis=1)

        voltage = (voltage*3.3)/4096
        current = (current*33*2)/4096

        return voltage, current

    def _update(self):

        self.xdata, self.ydata = self.get_data()
        self.plot.setData(self.xdata, self.ydata)

        #DEBUG INFO
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
        print("DIODE App closed")
        self.socket.send(b'0')


if __name__ == '__main__':

    app = QtWidgets.QApplication(sys.argv)
    thisapp = App()
    thisapp.show()
    exit_code = app.exec()
    thisapp.__del__()
    sys.exit(exit_code)