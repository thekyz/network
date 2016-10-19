static	= yes
#static  = no

ZMQLIB	= ../zmq_lib
CC		= g++
LDFLAGS	= -L${ZMQLIB}/lib
CFLAGS 	= -I${ZMQLIB}/include -g -o0 --std=c++11

ifeq ($(static),yes)
LDLIBS 	= -Wl,-Bstatic -lzmq -Wl,-Bdynamic -lpthread
else
LDLIBS 	= -lzmq
endif

all: broker player

broker: broker.c net.c

client: client.c

clean:
	@rm *.o -f
	@rm broker -f
	@rm client -f

