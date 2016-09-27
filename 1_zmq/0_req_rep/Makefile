static	= yes
#static  = no

ZMQLIB	= ../zmq_lib
CC		= g++
LDFLAGS	= -L${ZMQLIB}/lib
CFLAGS 	= -I${ZMQLIB}/include

ifeq ($(static),yes)
LDLIBS 	= -Wl,-Bstatic -lzmq -Wl,-Bdynamic -lpthread
else
LDLIBS 	= -lzmq
endif

all: server client

server: server.c

client: client.c

clean:
	@rm *.o -f
	@rm client -f
	@rm server -f

