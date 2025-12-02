CC=gcc
CFLAGS=-Wall -Wextra -g

all: myfs viewer

myfs: main.o fs.o
	$(CC) $(CFLAGS) main.o fs.o -o myfs

viewer: viewer.o fs.o
	$(CC) $(CFLAGS) viewer.o fs.o -lncurses -o viewer

main.o: main.c fs.h
	$(CC) $(CFLAGS) -c main.c

fs.o: fs.c fs.h
	$(CC) $(CFLAGS) -c fs.c

viewer.o: viewer.c fs.h
	$(CC) $(CFLAGS) -c viewer.c

clean:
	rm -f *.o myfs viewer
