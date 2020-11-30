all: gradebookadd gradebookdisplay setup

CFLAGS=-g
LFLAGS=-lssl -lcrypto

setup: setup.o crypto.o
	$(CC) $(CFLAGS) -o setup setup.o crypto.o $(LFLAGS)

gradebookadd: gradebookadd.o crypto.o
	$(CC) $(CFLAGS) -o gradebookadd gradebookadd.o crypto.o $(LFLAGS)

gradebookdisplay: gradebookdisplay.o crypto.o
	$(CC) $(CFLAGS) -o gradebookdisplay gradebookdisplay.o crypto.o $(LFLAGS)

setup.o: setup.c
	$(CC) $(CFLAGS) -c -o setup.o setup.c

gradebookadd.o: gradebookadd.c
	$(CC) $(CFLAGS) -c -o gradebookadd.o gradebookadd.c

gradebookdisplay.o: gradebookdisplay.c
	$(CC) $(CFLAGS) -c -o gradebookdisplay.o gradebookdisplay.c

crypto.o: crypto.c
	$(CC) $(CFLAGS) -c -o crypto.o crypto.c



testing: testing.o crypto.o
	$(CC) $(CFLAGS) -o testing testing.o crypto.o $(LFLAGS)

testing.o: testing.c
	$(CC) $(CFLAGS) -c -o testing.o testing.c


testing_2: testing_2.o crypto.o
	$(CC) $(CFLAGS) -o testing_2 testing_2.o crypto.o $(LFLAGS)

testing_2.o: testing_2.c
	$(CC) $(CFLAGS) -c -o testing_2.o testing_2.c


clean:
	rm -f *.o
	rm -rf setup gradebookadd gradebookdisplay testing

clean_bin:
	rm -f *.bin