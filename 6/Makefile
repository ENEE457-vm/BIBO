all: gradebookadd gradebookdisplay setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto 

gradebookadd: gradebookadd.o data.o crypto.o
	$(CC) -g -o gradebookadd gradebookadd.o data.o crypto.o $(LFLAGS)

gradebookdisplay: gradebookdisplay.o data.o crypto.o
	$(CC) $(CFLAGS) -o gradebookdisplay gradebookdisplay.o data.o crypto.o $(LFLAGS)

setup: setup.o data.o crypto.o
	$(CC) $(CFLAGS) -o setup setup.o data.o crypto.o $(LFLAGS)

gradebookadd.o: gradebookadd.c data.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c data.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

setup.o: setup.c data.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c

data.o: data.c data.h
	$(CC) $(CFLAGS) -c -o data.o data.c

crypto.o: crypto.c
	$(CC) $(CFLAGS) -c -o crypto.o crypto.c

clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup crypto