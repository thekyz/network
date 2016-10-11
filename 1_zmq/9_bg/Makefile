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

all: dealer player

dealer: dealer.c dealer_net.c dealer_game.c

player: player.c

clean:
	@rm *.o -f
	@rm dealer -f
	@rm player -f

