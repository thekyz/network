import zmq

def main():
    context = zmq.Context()
    
    print('[C] Connecting to server ...')
    socket = context.socket(zmq.SUB)
    socket.connect('tcp://localhost:5556')
    
    msg_filter = u'deal'
    
    socket.setsockopt_string(zmq.SUBSCRIBE, msg_filter)
    
    while True:
        msg = socket.recv_string()
        _, rank, suit = msg.split()
        print('[C] ' + rank + ' of ' + suit)

if __name__ == '__main__':
    main()

