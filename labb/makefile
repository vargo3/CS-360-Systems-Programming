#CS 360 Lab 1 Makefile

CC = gcc 

CFLAGS = -lpthread -I/home/plank/cs360/include

EXECUTABLES = bonding

SOURCES = $(EXECUTABLES).c bonding-driver.c /home/plank/cs360/objs/libfdr.a

all: 
	$(CC) $(CFLAGS) -o $(EXECUTABLES) $(SOURCES)

#make clean will rid your directory of the executable,
#object files, and any core dumps you've caused
clean:
	rm core $(EXECUTABLES) *.o


