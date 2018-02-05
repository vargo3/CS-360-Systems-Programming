#CS 360 Lab 1 Makefile

CC = gcc 

CFLAGS =

EXECUTABLES = jsh1 jsh2 jsh3

all: $(EXECUTABLES)

jsh:
	$(CC) $(CFLAGS) -o $(EXECUTABLES) $(EXECUTABLES).c

#make clean will rid your directory of the executable,
#object files, and any core dumps you've caused
clean:
	rm core $(EXECUTABLES) *.o


