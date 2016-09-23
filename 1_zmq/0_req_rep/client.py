import zmq

context = zmq.Context()

print('[C] Connecting to server ...')
socket = context.socket(zmq.REQ)
socket.connect('tcp://localhost:5555')

socket.send(b'hello') 

message = socket.recv()
print('[C] << ' + message)

