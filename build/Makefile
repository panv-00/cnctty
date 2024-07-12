CC=gcc
PKGS=
CFLAGS=-Wall -O3 -g -lssl -lcrypto -pthread
LIBS=
VPATH=../src

OBJECTS= \
	cnc_library.o \
	cnc_net.o \
	cnc_message.o \

DEPLIST= \

default: $(OBJECTS)
	$(CC) -o ../cnctty ../src/cnctty.c \
		$(OBJECTS) \
		$(CFLAGS) \
		$(LIBS)
	
%.o: %.c %.h $(DEPLIST)
	$(CC) -c $< -o $*.o $(CFLAGS)
