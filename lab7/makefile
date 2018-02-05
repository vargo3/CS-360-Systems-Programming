#CS 360 Lab 1 Makefile

CC = gcc 

CFLAGS = -m32

EXECUTABLES = mymalloc

all: $(EXECUTABLES)

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $*.c

jtar: jtar.o
	$(CC) $(CFLAGS) -o mymalloc mymalloc.o $(LIBS)

#make clean will rid your directory of the executable,
#object files, and any core dumps you've caused
clean:
	rm core $(EXECUTABLES) *.o


