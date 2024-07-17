CC=gcc
CFLAGS=-Wall -O3 -g -lssl -lcrypto -pthread
VPATH=../src:../src/cnc_library

OBJECTS= \
	cnc_library.o \
	cnc_net.o \
	cnc_message.o \

default: $(OBJECTS)
	$(CC) -o ../cnctty ../src/cnctty.c \
		$(OBJECTS) \
		$(CFLAGS)
	
%.o: ../src/%.c ../scr/%.h
	$(CC) -c $< -o $*.o

cnc_library.o: cnc_library/cnc_library.c cnc_library/cnc_library.h
	$(CC) -c $< -o $*.o
