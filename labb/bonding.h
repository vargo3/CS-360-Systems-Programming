/* CS360 Lab B - Bonding.
   Header file bonding.h
   James S. Plank
   April, 2017.

   This file contains the common material and prototypes
   for the Bonding lab.

*/

#include <pthread.h>

/* This is the argument to hydrogen and oxygen threads. */

struct bonding_arg {
  int id;
  void *v;
};

/* You are to write these three procedures. */

void *initialize_v(char *verbosity);
void *hydrogen(void *arg);
void *oxygen(void *arg);

/* When two hydrogen threads and one oxygen thread have bonded
   to create a water molecule, each of them should call Bond()
   with the exact same arguments. This will return a string.
   Each thread should return the string. */

char *Bond(int h1, int h2, int o);

/* I've written these for you.  They are convenient: */

pthread_mutex_t *new_mutex();             /* Allocates, initializes and returns a new mutex. */
pthread_cond_t  *new_cond();              /* Allocates, initializes and returns a new condition variable. */
void print_state(char c, const char *s);  /* Prints the character, the system state, and the string. */

