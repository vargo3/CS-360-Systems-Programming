#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bonding.h"
#include "jrb.h"
#include "dllist.h"

void usage(char *s)
{
  fprintf(stderr, "usage: bonding seed num_molecules max_outstanding verbosity\n");
  if (s != NULL) fprintf(stderr, "%s\n", s);
  exit(1);
}

/* This allocates and creates a new mutex. */

pthread_mutex_t *new_mutex()
{
  pthread_mutex_t *l;

  l = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  if (l == NULL) { perror("malloc pthread_mutex_t"); exit(1); }
  pthread_mutex_init(l, NULL);
  return l;
}

/* This allocates and creates a new condition variable. */

pthread_cond_t *new_cond()
{
  pthread_cond_t *l;

  l = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  if (l == NULL) { perror("malloc pthread_cond_t"); exit(1); }
  pthread_cond_init(l, NULL);
  return l;
}

/* Each thread has a pointer to one of these.  It is stored
   on two red-black trees, named tid_to_info and number_to_info.
   Those are described below. */

struct thread_info {
  pthread_t tid;         /* This is the thread's pthread_t, as returned from pthread_create(). */
  Jval jv;               /* This is the pthread_t copied to the .l field of a jval.  It facilitates tid_to_info. */
  int number;            /* Each thread has unique number, starting from 0 and incrementing. */
  int type;              /* 'o' for Oxygen. 'h' for Hydrogen */
  char *bond_id;         /* When the thread calls Bond(), this is the return value of the Bond() call. */
  struct bonding_arg ba; /* This is the argument passed to the new thread via pthread_create(). */
};

/* We store the pthread_t's as jvals.  We memcpy the pthread_t to a jval.  The reason is
   that the compiler can be restrictive, and not allow you to use pthread_t's.  This is my
   way of getting around that.  */

int jvcmp(Jval v1, Jval v2)
{
  if (v1.l < v2.l) return -1;
  if (v1.l > v2.l) return 1;
  return 0;
}

/* This is my global information about the system.  There is just one of these, and
   it is not visible outside of this program (i.e. the procedures in bonding.c
   cannot make use of this. */

struct global_info {

  int num_molecules;        /* Total number of molecules that will be created. */
  int max_outstanding;      /* Total number of outstanding threads allowed. */

  int molecules_created;    /* Total number of molecules that have been created. */
  int threads_forked;       /* Total number of threads that have been created. */
  int threads_joined;       /* Total number of threads that have been joined. */
  int h_spawned;            /* Total number of 'h' threads that have been created. */
  int o_spawned;            /* Total number of 'o' threads that have been created. */
  char *verbosity;          /* The verbosity argument. */

  JRB tid_to_info;          /* Tree that translates pthread_t's to thread_info structs. */
  JRB number_to_info;       /* Tree that translates numbers to info thread_structs. */
  Dllist bonds;             /* List of numbers of threads that have called Bond(), but 
                               have not been joined yet.  This is appended in Bond(), 
                               and then read/deleted in the joiner thread. */

  pthread_mutex_t *lock;         /* Lock for this data structure.  I could get creative, and have different
                                    locks for different parts (e.g. one for bonds, one for creation), but
                                    I'm keeping it simple. */

  pthread_cond_t  *joiner_cond;  /* This is the condition variable that the joiner waits upon.
                                    It is signaled when the first thread of a molecule calls Bond().
                                    It is at that point that the three atoms in the molecule are
                                    appended to bonds, with the last of these being the oxygen atom. */

  pthread_cond_t  *create_cond;  /* This is the condition variable that the creation thread waits upon.
                                    It is signaled by the joiner thread after each join. */

  void *v;                       /* The void * for bonding.c */
};

static struct global_info *G;

/* print_state prints the state of the system, along with a character and a string. */

void print_state(char c, const char *s)
{
  char buf[1000];

  sprintf(buf, "%c: TF: %4d - TJ: %4d - HF: %4d - OF: %4d - %s\n", 
          c, G->threads_forked, G->threads_joined, 
          G->h_spawned, G->o_spawned, s);
  printf("%s", buf);
  fflush(stdout);
}

/* This is the thread that creates hydrogen and oxygen threads.
   It does this in batches.  It makes sure that at most
   max_outstanding threads are outstanding, and it doesn't start
   creating threads until the number outstanding threads is <=
   max_outstanding/2. It also keeps the number of h and o threads
   in balance. */

void *creator_thread(void *arg)
{
  struct thread_info *ti;
  int total_to_be_spawned, h_to_create, o_to_create;
  int outstanding_threads, threads_to_create;
  int i;
  char buf[200];

  while (1) {
    pthread_mutex_lock(G->lock);
    
    /* If there are too many outstanding threads, then wait until
       more threads have joined with the joiner thread. */

    outstanding_threads = G->threads_forked - G->threads_joined;
    if (outstanding_threads > G->max_outstanding/2) {
      if (strchr(G->verbosity, 'C') != NULL) {
        print_state('C', "Creator Thread Sleeping");
      }
      pthread_cond_wait(G->create_cond, G->lock);

    /* Otherwise, create enough threads so that there are 
       max_outstanding threads.  Make sure that the h/o balance is
       correct.   */

    } else {
      threads_to_create = (G->max_outstanding - outstanding_threads);
      if (threads_to_create > G->num_molecules * 3 - G->threads_forked) {
        threads_to_create = G->num_molecules * 3 - G->threads_forked;
      }
      total_to_be_spawned = threads_to_create + G->h_spawned + G->o_spawned;
      h_to_create = ((total_to_be_spawned * 2) / 3) - G->h_spawned;
      o_to_create = (total_to_be_spawned / 3 ) - G->o_spawned;

      if (h_to_create + o_to_create < threads_to_create) h_to_create++;
      if (h_to_create + o_to_create < threads_to_create) o_to_create++;

      for (i = 0; i < threads_to_create; i++) {
        ti = (struct thread_info *) malloc(sizeof(struct thread_info));
        if (ti == NULL) { perror("malloc thread_info"); exit(1); }
        ti->number = G->threads_forked;
        ti->ba.id = ti->number;
        ti->ba.v = G->v;
        ti->bond_id = NULL;
        if (o_to_create > 0 && (rand()%3 == 0 || h_to_create == 0)) {
          ti->type = 'o';
          o_to_create--;
          G->o_spawned++;
          if (pthread_create(&(ti->tid), NULL, oxygen, (void *) &(ti->ba)) != 0) {
            perror("pthread_create(oxygen)"); 
            exit(1);
          }
        } else {
          ti->type = 'h';
          h_to_create--;
          G->h_spawned++;
          if (pthread_create(&(ti->tid), NULL, hydrogen, (void *) &(ti->ba)) != 0) {
            perror("pthread_create(oxygen)"); 
            exit(1);
          }
        }

        /* Update stats and the two trees. */

        G->threads_forked++;

        memcpy(&(ti->jv.l), &(ti->tid), sizeof(pthread_t));
        jrb_insert_gen(G->tid_to_info, ti->jv, new_jval_v((void *) ti), jvcmp);

        jrb_insert_int(G->number_to_info, ti->number, new_jval_v((void *) ti));

        if (strchr(G->verbosity, 'C') != NULL) {
          sprintf(buf, "Created thread with tid 0x%lx.  Number: %d.  Type: %c",
                  ti->jv.l, ti->number, ti->type);
          print_state('C', buf);
        }
      }
    }

    /* Check for the exit condition. */

    if (G->threads_forked == G->num_molecules * 3) {
      if (strchr(G->verbosity, 'C') != NULL) {
        print_state('C', "Creator thread is done.  Calling pthread_exit().");
      }
      pthread_mutex_unlock(G->lock);
      pthread_exit(NULL);
    } 

    /* Otherwise, unlock the mutex before trying again. */

    pthread_mutex_unlock(G->lock);
  }
}

/* The joiner thread both cleans up the threads, and double-checks their return
   values. It wakes up when there are thread numbers in the bonds list.  Then,
   for each of these numbers, it calls pthread_join() on the proper
   tid, and double-checks that the return value of pthread_join()
   matches what it should.  If all is ok, and the thread is an oxygen
   thread, then it frees all of the thread_info strucs of the three
   threads that compose the molecule.  It works in this order, because
   the oxygen thread is the last of the three to be added to bonds.  */

void *joiner_thread(void *arg)
{
  int n, h1, h2, o;
  JRB tmp;
  struct thread_info *ti, *h;
  char *rv;
  char buf[200];

  pthread_mutex_lock(G->lock);
  while (1) {

    /* If there is a thread number in bonds, then take it off the list. */

    while (!dll_empty(G->bonds)) {
      n = G->bonds->flink->val.i;
      dll_delete_node(G->bonds->flink);

      /* Get the thread_info struct for the thread. */

      tmp = jrb_find_int(G->number_to_info, n);
      if (tmp == NULL) {
        fprintf(stderr, "ERROR - thread number is not in number_to_info.\n");
        exit(1);
      }
      ti = (struct thread_info *) tmp->val.v;

      /* Time to call pthread_join.  You release the mutex before you do that. */

      pthread_mutex_unlock(G->lock);
 
      if (strchr(G->verbosity, 'J') != NULL) {
        sprintf(buf, "Joining with thread number %d.  Tid 0x%lx (Bond %s)", ti->number, ti->jv.l, ti->bond_id);
        print_state('J', buf);
      }

      pthread_join(ti->tid, (void **) (&rv));

      /* Now, check the return value of the thread. */

      if (rv == NULL) {
        fprintf(stderr, "Error on pthread_join of thread %d.  Rv should be %s, but it was NULL.\n",
           ti->number, ti->bond_id);
        exit(1);
      }
      if (memcmp(rv, ti->bond_id, strlen(ti->bond_id)+1) != 0) {
        fprintf(stderr, "Error on pthread_join of thread %d.  Rv should be %s, but it wasn't.\n",
           ti->number, ti->bond_id);
        fprintf(stderr, "  I don't want to print it out, because it may be junk.\n");
        exit(1);
      } 

      /* Reacquire the lock, Update the information about the thread, and  if the thread is an 
         oxygen thread, free the thread_infos and their entries in number_to_info for all three
         atoms in the molecule. */

      pthread_mutex_lock(G->lock);
      G->threads_joined++;

      if (strchr(G->verbosity, 'J') != NULL) {
        sprintf(buf, "Join complete and verified: thread number %d.  Bond %s.", ti->number, ti->bond_id);
        print_state('J', buf);
      }
      
      if (ti->type == 'o') {
        sscanf(ti->bond_id, "(%d,%d,%d)", &h1, &h2, &o);
        jrb_delete_node(tmp);
        tmp = jrb_find_int(G->number_to_info, h1);
        if (tmp == NULL) { fprintf(stderr, "ERROR H1-NO-NUMBER\n"); exit(1); }
        h = (struct thread_info *) tmp->val.v;
        free(h->bond_id);
        free(h);
        jrb_delete_node(tmp);
        tmp = jrb_find_int(G->number_to_info, h2);
        if (tmp == NULL) { fprintf(stderr, "ERROR H2-NO-NUMBER\n"); exit(1); }
        h = (struct thread_info *) tmp->val.v;
        free(h->bond_id);
        free(h);
        jrb_delete_node(tmp);
        free(ti->bond_id);
        free(ti);
      }

      /* Signal the creator thread, since we have reduced the number of
         outstanding threads. */

      pthread_cond_signal(G->create_cond);
    }

    /* If we're done, double-check that there's no stray information  
       in the red-black trees, and then exit. */

    if (G->threads_joined == G->num_molecules * 3) {
      if (strchr(G->verbosity, 'J') != NULL) {
        sprintf(buf, "The joiner is done.  Calling pthread_exit().");
        print_state('J', buf);
      }
      if (!jrb_empty(G->tid_to_info)) {
        fprintf(stderr, "ERROR - Tid tree is not empty.\n");
        exit(1);
      }
      if (!jrb_empty(G->number_to_info)) {
        fprintf(stderr, "ERROR - Number tree is not empty.\n");
        exit(1);
      }
      pthread_mutex_unlock(G->lock);
      pthread_exit(NULL);
    }

    /* At this point, there are no more threads to join, (G->bonds 
       is empty), so block. */

    pthread_cond_wait(G->joiner_cond, G->lock);
  }
  
}

/* The main is pretty straightfoward, setting up variables, and
   forking off the creator and joiner threads. */

int main(int argc, char **argv)
{
  int seed;
  struct global_info g;
  struct thread_info *ti;
  pthread_t creator, joiner;

  if (sizeof(pthread_t) != 8) {
    fprintf(stderr, "Sizeof(pthread_t) needs to be 8 (it is %d).  Sorry.\n", 
            (int) sizeof(pthread_t));
    exit(1);
  }

  G = &g;

  if (argc != 5) usage(NULL);
  seed = atoi(argv[1]);
  G->num_molecules = atoi(argv[2]);
  if (G->num_molecules > 715827880 || G->num_molecules <= 0) {
    usage("num_molecules has to be between 1 and 715827880.");
  }
  G->max_outstanding = atoi(argv[3]);
  if (G->num_molecules == 1 && G->max_outstanding != 3) {
    usage("If num_molecules, then max_outstanding must be 3.");
  }
  if (G->num_molecules > 1 && (G->max_outstanding > 1000 || G->max_outstanding < 6)) {
    usage("max_outstanding has to be between 6 and 1000.");
  }

  G->verbosity = argv[4];

  srand(seed);

  G->joiner_cond = new_cond();

  G->lock = new_mutex();
  G->create_cond = new_cond();

  G->molecules_created = 0;
  G->threads_joined = 0;
  G->threads_forked = 0;
  G->h_spawned = 0;
  G->o_spawned = 0;

  G->tid_to_info = make_jrb();
  G->number_to_info = make_jrb();
  G->bonds = new_dllist();

  G->v = initialize_v(G->verbosity);

  if (pthread_create(&creator, NULL, creator_thread, NULL) != 0) {
    perror("pthread_create creator");
    exit(1);
  }

  if (pthread_create(&joiner, NULL, joiner_thread, NULL) != 0) {
    perror("pthread_create joiner");
    exit(1);
  }

  pthread_exit(NULL);
}
  
/* This sets the bond_id variable of the thread whose number a.  It error checks
   while doing this, and it does so by calling strdup().  In other words, even though
   three atoms get the same bond string, they all get their own copies, so that they
   can each free them.  It is assumed that we hold G->lock during this call. */

void set_bond(char *rv, int a, int number)
{
  JRB tmp;
  struct thread_info *ti;

  tmp = jrb_find_int(G->number_to_info, a);
  if (tmp == NULL) {
    fprintf(stderr, "Error.  Thread %d called Bond%s, and thread number %d is not in the database.\n",  number, rv, a);
    exit(1);
  }
  ti = (struct thread_info *) tmp->val.v;
  if (ti->bond_id != NULL) {
    fprintf(stderr, "Error.  Thread %d called Bond%s, and thread number %d already has a bond %s.\n",  number, rv, a, ti->bond_id);
    exit(1);
  }
  ti->bond_id = strdup(rv);
}

/* This is what threads call when they have discovered a Bond.  It checks to make sure
   that the calling thread is in fact one of the ones specified in h1, h2 or o.
   It also deletes the pthread_t from tid_to_info.  If this is the first call for the
   molecule, then it sets all of the bond_id strings for the three thread_info's, and
   it appends all three atoms to G->bonds, ending with the oxygen thread. Finally,
   it returns the bond string to the caller. */
   
char *Bond(int h1, int h2, int o)
{
  char rv[60];
  pthread_t tid;
  Jval jv;
  JRB tmp;
  struct thread_info *ti;
  int th1, th2, to;
  char buf[200];

  tid = pthread_self();
  pthread_mutex_lock(G->lock);
  memcpy(&(jv.l), &tid, sizeof(pthread_t));
  tmp = jrb_find_gen(G->tid_to_info, jv, jvcmp);
  if (tmp == NULL) {
    fprintf(stderr, "Error -- pthread_t 0x%lx called Bond(%d,%d,%d), but the pthread_t is no longer in the pthread_t database.\n", jv.l, h1, h2, o);
    exit(1);
  }

  ti = (struct thread_info *) tmp->val.v;
  jrb_delete_node(tmp);
    
  if (ti->number != h1 &&
      ti->number != h2 &&
      ti->number != o) {
    fprintf(stderr, "Error -- thread with id %d called Bond(%d,%d,%d).  The id has to be one of the atoms.\n", ti->number, h1, h2, o);
    exit(1);
  }

  if (h1 == h2 || h1 == o || h2 == o) {
    fprintf(stderr, "Error -- thread with id %d called Bond(%d,%d,%d).  Can't specify duplicate atoms.\n", ti->number, h1, h2, o);
    exit(1);
  }

  if ((ti->type == 'h' && ti->number == o) || (ti->type == 'o' && ti->number != o)) {
    fprintf(stderr, "Error -- thread with id %d called Bond(%d,%d,%d).  It is of the wrong type for this bond.\n", ti->number, h1, h2, o);
    exit(1);
  }

  if (ti->bond_id != NULL) {
    if (sscanf(ti->bond_id, "(%d,%d,%d)", &th1, &th2, &to) != 3) {
      fprintf(stderr, "Internal error sscanf(ti->bond_id).  I'd say that memory is corrupted\n");
      exit(1);
    }
    if (th1 != h1 || th2 != h2 || to != o) {
      fprintf(stderr, "Error -- thread with id %d called Bond(%d,%d,%d), but another thread had that atom in a bond: %s.\n", ti->number, h1, h2, o, ti->bond_id);
      exit(1);
    }
  } else {
    G->molecules_created++;
    sprintf(rv, "(%d,%d,%d)", h1, h2, o);

    set_bond(rv, h1, ti->number);
    set_bond(rv, h2, ti->number);
    set_bond(rv, o, ti->number);

    dll_append(G->bonds, new_jval_i(h1));
    dll_append(G->bonds, new_jval_i(h2));
    dll_append(G->bonds, new_jval_i(o));
    pthread_cond_signal(G->joiner_cond);
  }
  
  if (strchr(G->verbosity, 'B') != NULL) {
    sprintf(buf, "Thread %d called Bond%s.", ti->number, ti->bond_id);
    print_state('B', buf);
  }
  pthread_mutex_unlock(G->lock);

  return ti->bond_id;
}
