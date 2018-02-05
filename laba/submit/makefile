#CS 360 Lab 1 Makefile

CC = gcc 

CFLAGS = -I/home/plank/cs360/notes/Sockets -lpthread

EXECUTABLES = chat_server

SOURCES = $(EXECUTABLES).c /home/plank/cs360/notes/Sockets/sockettome.c

all: 
	$(CC) $(CFLAGS) -o $(EXECUTABLES) $(SOURCES)

#make clean will rid your directory of the executable,
#object files, and any core dumps you've caused
clean:
	rm core $(EXECUTABLES) *.o


