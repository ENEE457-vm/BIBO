CC=gcc
CFLAGS=-Wall -Iincludes -z noexecstack -fstack-protector
LDLIBS=-lcrypto

all: setup gradebookadd gradebookdisplay

%.o: %.c 
	$(CC) -c -o $@ $< $(CFLAGS)

setup: setup.o
	$(CC) -o setup setup.o $(LDLIBS)

gradebookadd: gradebookadd.o
	$(CC) -o gradebookadd gradebookadd.o $(LDLIBS)

gradebookdisplay: gradebookdisplay.o
	$(CC) -o gradebookdisplay gradebookdisplay.o $(LDLIBS)

.PHONY : clean all

clean:
	rm -f *~ *.o setup gradebookadd gradebookdisplay