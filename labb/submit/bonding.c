#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "bonding.h"

struct list_node{
	int id;
	int h1;
	int h2;
	int o;
	pthread_cond_t *cond;
	struct list_node *flink;
	struct list_node *blink;
};

struct globals{
	struct list_node *h;
	struct list_node *o;
	pthread_mutex_t *lock;
	char *verbosity;
};

void *initialize_v(char *verbosity)
{
	struct globals *g;

	g = (struct globals *) malloc(sizeof(struct globals));
	g->h = NULL;
	g->o = NULL;
	g->lock = new_mutex();
	g->verbosity = verbosity;
	return g;
}

void *hydrogen(void *arg)
{
	struct bonding_arg *a;
	struct globals *g;
	struct list_node me, *node;

	a = (struct bonding_arg *) arg;
	g = (struct globals *) a->v;
	me.id = a->id;
	me.h1 = -1;
	me.h2 = -1;
	me.o = -1;
	me.cond = new_cond();
	me.flink = NULL;
	me.blink = NULL;

	pthread_mutex_lock(g->lock);
	if (g->o != NULL && g->h != NULL){
		//save ids for each atom
		if (strchr(g->verbosity, 'A') != NULL)
			printf("Hydrogen Thread %d found bond\n", me.id);
		me.h1 = g->h->id;
		g->h->h1 = g->h->id;
		g->o->h1 = g->h->id;
		me.h2 = me.id;
		g->h->h2 = me.id;
		g->o->h2 = me.id;
		me.o = g->o->id;
		g->h->o = g->o->id;
		g->o->o = g->o->id;

		pthread_cond_signal(g->h->cond);
		pthread_cond_signal(g->o->cond);

		//remove g->h from h list
		g->h = g->h->flink;
		if (g->h != NULL) g->h->blink = NULL;
		//remove g->o from o list
		g->o = g->o->flink;
		if (g->o != NULL) g->o->blink = NULL;
		pthread_mutex_unlock(g->lock);
		free(me.cond);
		return (void *) Bond(me.h1, me.h2, me.o);
	}

	//add self to h list
	if (g->h == NULL)
		g->h = &me;
	else{
		node = g->h;
		while (node->flink != NULL) node = node->flink;
		node->flink = &me;
		me.blink = node;
	}
	if (strchr(g->verbosity, 'A') != NULL)
		printf("Hydrogen Thread %d waiting\n", me.id);
	pthread_cond_wait(me.cond, g->lock);
	if (strchr(g->verbosity, 'A') != NULL)
		printf("Hydrogen Thread %d being bonded\n", me.id);

	pthread_mutex_unlock(g->lock);
	free(me.cond);
	return (void *) Bond(me.h1, me.h2, me.o);
}

void *oxygen(void *arg)
{
	struct bonding_arg *a;
	struct globals *g;
	struct list_node me, *node;
	int i;

	a = (struct bonding_arg *) arg;
	g = (struct globals *) a->v;
	me.id = a->id;
	me.h1 = -1;
	me.h2 = -1;
	me.o = -1;
	me.cond = new_cond();
	me.flink = NULL;
	me.blink = NULL;

	pthread_mutex_lock(g->lock);
	if (g->h != NULL && g->h->flink != NULL){
		if (strchr(g->verbosity, 'A') != NULL)
			printf("Oxygen Thread %d found bond\n", me.id);
		//save ids for each atom
		me.h1 = g->h->id;
		g->h->h1 = g->h->id;
		g->h->flink->h1 = g->h->id;
		me.h2 = g->h->flink->id;
		g->h->h2 = g->h->flink->id;
		g->h->flink->h2 = g->h->flink->id;
		me.o = me.id;
		g->h->o = me.id;
		g->h->flink->o = me.id;

		pthread_cond_signal(g->h->cond);
		pthread_cond_signal(g->h->flink->cond);

		//remove g->h and g->h->flink from h list
		g->h = g->h->flink;
		g->h->blink = NULL;
		g->h = g->h->flink;
		if (g->h != NULL) g->h->blink = NULL;

		pthread_mutex_unlock(g->lock);
		free(me.cond);
		return (void *) Bond(me.h1, me.h2, me.o);
	}

	//add self to o list
	if (g->o == NULL)
		g->o = &me;
	else{
		node = g->o;
		while (node->flink != NULL) node = node->flink;
		node->flink = &me;
		me.blink = node;
	}
	//wait until signaled
	if (strchr(g->verbosity, 'A') != NULL)
		printf("Oxygen Thread %d waiting\n", me.id);
	pthread_cond_wait(me.cond, g->lock);
	//self has already been removed from o list
	pthread_mutex_unlock(g->lock);
	if (strchr(g->verbosity, 'A') != NULL)
		printf("Oxygen Thread %d being bonded\n", me.id);
	free(me.cond);
	return (void *) Bond(me.h1, me.h2, me.o);
}
