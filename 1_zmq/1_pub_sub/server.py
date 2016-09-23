import zmq
import random
import time

SUITS = [ 'clubs', 'diamonds', 'hearts', 'spades' ]

def main():
    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.bind('tcp://*:5556')
    
    print('[S] Publishing ...')
    
    while True:
        suit = random.randrange(0, 3)
        rank = random.randrange(1, 13)
    
        socket.send_string('deal ' + str(rank) + ' ' + SUITS[suit])

        time.sleep(1)

if __name__ =='__main__':
    main()

