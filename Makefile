
CC?= gcc
CFLAGS= -O2 -Wall
BINDIR= ./bin/
SRCDIR= ./src/
OBJDIR= ./build/

# Uncomment to send raw packet output to stdout and enable debugging
#CFLAGS+= -D_DEBUG -O0 -g

# Define a custom port to point the client to here (as an integer)
#CFLAGS+= -DMIDDLEMAN_PORT=5998

# Define custom login server hostname and port here (both as strings)
#CFLAGS+= -DREMOTE_HOST=\"login.eqemulator.net\"
#CFLAGS+= -DREMOTE_PORT=\"5998\"

_OBJS= main.o connection.o protocol.o sequence.o
_DEPS= connection.h protocol.h sequence.h netcode.h errors.h

OBJS= $(patsubst %,$(OBJDIR)%,$(_OBJS))
DEPS= $(patsubst %,$(SRCDIR)%,$(_DEPS))

default all: $(BINDIR)p99-login-middlemand

$(BINDIR)p99-login-middlemand: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJDIR)%.o: $(SRCDIR)%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(OBJDIR)*.o
