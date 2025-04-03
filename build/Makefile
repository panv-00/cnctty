CC=gcc
CFLAGS=-Wall -O3 -g
LFLAGS=-lssl -lcrypto
VPATH=../src:../src/cnc_library

OBJECTS= \
	cnc_library.o \
	cnc_net.o \
	cnc_message.o \

default: $(OBJECTS)
	$(CC) -o ../cnctty ../src/cnctty.c \
		$(OBJECTS) \
		$(CFLAGS) \
		$(LFLAGS)
	
%.o: ../src/%.c ../src/%.h
	$(CC) -c $< -o $*.o

cnc_library.o: cnc_library/cnc_library.c cnc_library/cnc_library.h
	$(CC) -c $< -o $*.o
