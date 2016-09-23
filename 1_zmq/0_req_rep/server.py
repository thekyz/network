import time
import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind('tcp://*:5555')

print('[S] Listening ...')

while True:
    message = socket.recv()
    print('[S] << ' + message)

    time.sleep(1)

    socket.send(b'world')

