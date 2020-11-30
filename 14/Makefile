all: gradebookadd gradebookdisplay setup

CFLAGS=-g 
LFLAGS=-lssl -lcrypto 

gradebookadd: 
	$(CC) -g -o gradebookadd gradebookadd.c $(LFLAGS)

gradebookdisplay: 
	$(CC) $(CFLAGS) -g -o gradebookdisplay gradebookdisplay.c $(LFLAGS)

setup: 
	$(CC) $(CFLAGS) -o setup setup.c $(LFLAGS)

data: 
	$(CC) $(CFLAGS) -o data data.c $(LFLAGS)
# gradebookadd_skel.o: gradebookadd_skel.c
# 	$(CC) $(CFLAGS) -c -o gradebookadd_skel.o gradebookadd_skel.c

# gradebookdisplay_skel.o: gradebookdisplay_skel.c
# 	$(CC) $(CFLAGS) -c -o gradebookdisplay_skel.o gradebookdisplay_skel.c

# setup.o: setup_skel.c
# 	$(CC) $(CFLAGS) -c -o setup_skel.o setup_skel.c

# data_skel.o: data_skel.c
# 	$(CC) $(CFLAGS) -c -o data_skel.o data_skel.c

clean:
	rm -f *.o
	rm -rf gradebookadd gradebookdisplay setup