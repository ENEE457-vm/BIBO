CC = gcc
CFLAGS = -Wall -fstack-protector-all -g3 -O0 -Wno-varargs
LDFLAGS = -lssl -lcrypto
TARGETS = setup gradebookadd gradebookdisplay

.PHONY: all clean

all: $(TARGETS)

setup: gradebook.o gradebook_crypto.o setup.o
	$(CC) $^ -o $@ $(LDFLAGS)

gradebookadd: gradebook.o gradebook_crypto.o gradebookadd.o
	$(CC) $^ -o $@ $(LDFLAGS)

gradebookdisplay: gradebook.o gradebook_crypto.o gradebookdisplay.o
	$(CC) $^ -o $@ $(LDFLAGS)

tests: tests.o gradebook.o gradebook_crypto.o
	$(CC) $^ -o $@ -lcmocka $(LDFLAGS) -Wl,--wrap=printf

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) tests *.o
