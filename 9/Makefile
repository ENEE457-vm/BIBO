all: gradebookadd gradebookdisplay setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto -lm 

gradebookadd: gradebookadd.o data.o input.o
	$(CC) -g -o gradebookadd gradebookadd.o data.o input.o $(LFLAGS)

gradebookdisplay: gradebookdisplay.o data.o input.o
	$(CC) $(CFLAGS) -o gradebookdisplay gradebookdisplay.o data.o input.o $(LFLAGS)

setup: setup.o data.o input.o
	$(CC) $(CFLAGS) -o setup setup.o data.o input.o $(LFLAGS)

gradebookadd.o: gradebookadd.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

setup.o: setup.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c

data.o: data.c
	$(CC) $(CFLAGS) -c -o data.o data.c

input.o: input.c
	$(CC) $(CFLAGS) -c -o input.o input.c

clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup
