all: gradebookadd gradebookdisplay setup
# all: logappend logread setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto 
CC=gcc

gradebookadd: gradebookadd.o 
	$(CC) -g -o gradebookadd gradebookadd.o  $(LFLAGS)


gradebookdisplay: gradebookdisplay.o
	$(CC) -g -o gradebookdisplay gradebookdisplay.o  $(LFLAGS)


setup: setup.o 
	$(CC) $(CFLAGS) -o setup setup.o $(LFLAGS)

gradebookadd.o: gradebookadd.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

setup.o: setup.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c


clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup
