//Jacob Vargo
//cs360 lab 7
//Description:
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mymalloc.h"

typedef struct flist{
	int size;
	struct flist *flink;
	struct flist *blink;
} *Flist;

Flist malloc_head = NULL;

void *my_malloc(size_t size){
	Flist ret;
	int tot_size;

	ret = NULL;
	if (size%8 != 0) size = size + 8 - (size%8) + 8; //round up size to a multiple of 8 bytes with an additional 8 bytes for bookkeeping
	else size += 8;
	if (malloc_head != NULL) tot_size = sbrk(0) - malloc_head;
	else tot_size = 0;
	if (tot_size >= size){
		ret = sbrk(size);
		((Flist)malloc_head)->blink = ret;
		((Flist)ret)->flink = malloc_head;
		((Flist)ret)->blink = NULL;
		malloc_head = ret;
	}
	else if (malloc_head == NULL){
		ret = sbrk(8192);
		((Flist)ret)->flink = NULL;
		((Flist)ret)->blink = NULL;
		malloc_head = ret;
	}

	// look through free list for a node that's at least size large
	
	return ret+8;
}

//put memory chunk back on free list
void my_free(void *ptr){
	Flist f;
	f = (Flist) (ptr-8); //ptr has 8 bytes before it allocated for bookkeeping
	f->flink = malloc_head;
	f->blink = NULL;
	malloc_head = f;
}

//return pointer to first node in free list
void *free_list_begin(){
	return malloc_head;
}

//return pointer to next node in free list
void *free_list_next(void *node){
	Flist f;
	f = (Flist) node;
	return f->flink;
}

void coalesce_free_list(){
	void *node;
	
	node = malloc_head;
	while (node != NULL){
		
		node = free_list_next(node);
	}
}

int main(){}
