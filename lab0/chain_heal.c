//Jacob Vargo
//Lab 0
//Description:
//read in 5 arguments from command line: initial_range, jump_range, num_jumps, initial_power, power_reduction.
//read in a file from stdin. each line of the file contains the info for each node.
//create an undirected graph from the input file. nodes are linked if they are within jump_range from each other.
//a node is said to be 'healed' x amount if it is in the path. x is equal to initial power for the first node in path and is reduced by power_reduction percentage for each subsequent node in path compounding. x can be at most the node's max health minus the current health.
//perform a DFS starting on all nodes within initial_range from "Urgosa_the_Healing_Shaman".
//find and print the path with the highest total healing.
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct node{
	char* name;
	int x, y;
	int cur_PP, max_PP;
	struct node **adj;
	int adj_size;
	int visited;
	struct node *prev;
} Node;

typedef struct{
	int num_jumps;
	int init_pow;
	double pow_rdct;
	int best_heal;
	Node **best_path;
	Node **cur_path;
	int *best_path_heal;
	int *cur_path_heal;
} Global;

void DFS(Node *cur, int hop_num, int tot_heal, Global *glo){
	int i;
	double heal;
	if (cur == NULL || glo == NULL || cur->visited == 1 || hop_num > glo->num_jumps || hop_num <= 0) return;
	cur->visited = 1;
	/* calculate how much cur can be healed */
	heal = glo->init_pow;
	for (i = 1; i < hop_num; i++) heal = heal * (1.0 - glo->pow_rdct);
	heal = rint(heal);
	if (heal > cur->max_PP - cur->cur_PP) heal = cur->max_PP - cur->cur_PP;
	tot_heal += heal;
	glo->cur_path[hop_num-1] = cur; //hop_num is off-by-one
	glo->cur_path_heal[hop_num-1] = heal;
	if (tot_heal > glo->best_heal){
		glo->best_heal = tot_heal;
		for (i = 0; i < glo->num_jumps; i++) glo->best_path[i] = glo->cur_path[i];
		for (i = 0; i < glo->num_jumps; i++) glo->best_path_heal[i] = glo->cur_path_heal[i];
	}
	for (i = 0; i < cur->adj_size; i++){
		DFS(cur->adj[i], hop_num+1, tot_heal, glo);
	}
	cur->visited = 0;
	glo->cur_path[hop_num-1] = NULL;
}

/* print names of all nodes wih their adj list*/
void PrintNodes(Node *cur){
	int i;
	while (cur != NULL){
		printf("%s\n", cur->name);
		for (i = 0; i < cur->adj_size; i++) printf("\t%s\n", cur->adj[i]->name);
		cur = (Node*) cur->prev;
	}
}

int main(int argc, char **argv){
	int init_rng, jump_rng;
	Global glo;
	int total_nodes;
	Node *last, *Urgosa; // last will always point to the last node in the singly-linked list of all the nodes.
	Node *cur, *cur2;
	Node **adj_nodes;
	Node **init_nodes;
	int init_nodes_size;
	int i;
	double dist;

	//read input
	sscanf(argv[1], "%d", &init_rng);
	sscanf(argv[2], "%d", &jump_rng);
	sscanf(argv[3], "%d", &glo.num_jumps);
	sscanf(argv[4], "%d", &glo.init_pow);
	sscanf(argv[5], "%lf", &glo.pow_rdct);
	last = NULL;
	cur = (Node*) malloc(sizeof(Node));
	cur->name = (char*) malloc(sizeof(char)*100);
	total_nodes = 0;
	/* create a singly-linked list from stdin. last will always point to the last node */
	while(scanf("%d %d %d %d %99s\n", &cur->x, &cur->y, &cur->cur_PP, &cur->max_PP, cur->name) != EOF){
		if (strcmp(cur->name, "Urgosa_the_Healing_Shaman", 100) == 0) Urgosa = cur;
		total_nodes++;
		cur->visited = 0;
		cur->prev = (struct node*) last;
		last = cur;
		cur = (Node*) malloc(sizeof(Node));
		cur->name = (char*) malloc(sizeof(char)*100);
	}
	free(cur->name);//the last malloc call is always extra
	free(cur);

	/* construct each node's adj list */
	adj_nodes = (Node**) malloc(sizeof(Node*) * total_nodes); //temp storage for the adj list being found
	init_nodes = (Node**) malloc(sizeof(Node*) * total_nodes); // stores all nodes within init_rng of Urgosa
	init_nodes[0] = Urgosa; //the loop will not find Urgosa casting on itself
	init_nodes_size = 1;
	cur = last;
	while (cur != NULL){
		cur->adj_size = 0;
		cur2 = last;
		while (cur2 != NULL){
			dist = ((cur->x - cur2->x) * (cur->x - cur2->x)) + ((cur->y - cur2->y) * (cur->y - cur2->y)); //dist is distance from cur to cur2 squared
			if (cur2 != cur){
				/* find nodes within init_rng, these are not added to the adj list */
				if (init_rng * init_rng >= dist && cur == Urgosa){
					init_nodes[init_nodes_size] = cur2;
					init_nodes_size++;
				}
				/* find nodes within jump_rng */
				if (jump_rng * jump_rng >= dist){
					adj_nodes[cur->adj_size] = cur2;
					cur->adj_size++;
				}
			}
			cur2 = (Node*) cur2->prev;
		}
		/* copy adj_nodes to the node's actually adj list now that the adj list size is known */
		cur->adj = (struct node**) malloc(sizeof(struct node*) * cur->adj_size);
		for (i = 0; i < cur->adj_size; i++) cur->adj[i] = adj_nodes[i];
		cur = (Node*) cur->prev;
	}
	free(adj_nodes);
	//PrintNodes(last);
	
	/* perform DFS on graph starting on Urgosa's adj list. Find path with largest healing */
	glo.best_heal = 0;
	glo.best_path = (Node**) malloc(sizeof(Node*) * glo.num_jumps);
	glo.cur_path = (Node**) malloc(sizeof(Node*) * glo.num_jumps);
	glo.best_path_heal = (int*) malloc(sizeof(int) * glo.num_jumps);
	glo.cur_path_heal = (int*) malloc(sizeof(int) * glo.num_jumps);
	for (i = 0; i < glo.num_jumps; i++){
		glo.best_path[i] = NULL;
		glo.cur_path[i] = NULL;
	}
	for (i = 0; i < init_nodes_size; i++){
		DFS(init_nodes[i], 1, 0, &glo);
	}

	/* Print path and heal amount */
	for (i = 0; i < glo.num_jumps; i++){
		if (glo.best_path[i] != NULL)
			printf("%s %d\n", glo.best_path[i]->name, glo.best_path_heal[i]);
	}
	printf("Total_Healing %d\n", glo.best_heal);

	return 0;
}
