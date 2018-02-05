//Jacob Vargo
//cs360 lab1
//Description:
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fields.h"
#include "jval.h"
#include "dllist.h"
#include "jrb.h"

struct person{
	char *name;
	char sex;
	struct person *mother;
	struct person *father;
	Dllist children;
	int visited;
};

struct person* CreatePerson(JRB people, char *name){
	//allocate space for and initialize a person
	struct person *p;
	p = malloc(sizeof(struct person));
	p->name = strdup(name);
	p->sex = 'U';
	p->father = NULL;
	p->mother = NULL;
	p->children = new_dllist();
	jrb_insert_str(people, p->name, new_jval_v(p));
	p->visited = 0;
	return p;
}

int DFS(struct person *p){
	//return 1 if p is its own decendent
	int i;
	Dllist t;
	if (p->visited == 1) return 0;
	if (p->visited == 2) return 1; //cycle found
	p->visited = 2;
	for (t = dll_first(p->children); t != dll_nil(p->children); t = dll_next(t)) //for each child of p
		if (DFS((struct person*) t->val.v)) return 1;
	p->visited = 1;
	return 0;
}

void PrintGraph(Dllist toprint){
	struct person *p, *c;
	Dllist t;
	while (!dll_empty(toprint)){
		//take p off the head of toprint
		p = (struct person*) dll_first(toprint)->val.v;
		dll_delete_node(dll_first(toprint));
		if (p->visited == 0){
			//check that any and all parents have been visited first before printing
			if ((p->mother == NULL && p->father == NULL) || (p->mother != NULL && p->mother->visited == 1 && p->father == NULL) || (p->mother == NULL && p->father != NULL && p->father->visited == 1) || (p->mother != NULL && p->mother->visited == 1 && p->father != NULL && p->father->visited == 1)){
				//print p
				printf("%s\n", p->name);
				printf("  Sex: ");
				if (p->sex == 'M') printf("Male\n");
				else if (p->sex == 'F') printf("Female\n");
				else printf("Unknown\n");
				printf("  Father: ");
				if (p->father != NULL) printf("%s\n", p->father->name);
				else printf("Unknown\n");
				printf("  Mother: ");
				if (p->mother != NULL) printf("%s\n", p->mother->name);
				else printf("Unknown\n");
				printf("  Children: ");
				if (!dll_empty(p->children)){
					printf("\n");
					for (t = dll_first(p->children); t != dll_nil(p->children); t = dll_next(t)){
						c = (struct person*) t->val.v;
						printf("    %s\n", c->name);
					}
				}
				else printf("None\n");
				printf("\n");
				p->visited = 1;
				for (t = dll_first(p->children); t != dll_nil(p->children); t = dll_next(t)) //for each child of p
					dll_append(toprint, t->val);
			}
		}
	}
}

void Genocide(JRB people){
	//free all the memory alocated within people including inside the person struct
	JRB node;
	struct person *p;
	jrb_traverse(node, people){
		p = (struct person*) node->val.v;
		free(p->name);
		free_dllist(p->children);
		free(p);
	}
	jrb_free_tree(people);
}

int main(int argc, char **argv){
	JRB people, node;
	struct person *p, *c;
	IS input;
	Dllist toprint;
	int i, j;
	char *w1, *w2;

	j=0;
	/* read input from stdin to a jrb using IS */
	input = new_inputstruct(NULL);
	if (input == NULL){
		perror("stdin");
		exit(0);
	}
	people = make_jrb();
	w2 = (char*) malloc(sizeof(char)*1000);
	while (get_line(input) >= 0){
		if (input->NF == 0) continue;
		//copy person into p
		w1 = input->fields[0];
		strcpy(w2, input->fields[1]);
		for (i = 2; i < input->NF; i++){
			strcat(w2, " ");
			strcat(w2, input->fields[i]);
		}
		if (strcmp(w1, "PERSON") == 0){
			if (NULL == jrb_find_str(people, w2)) p = CreatePerson(people, w2);
			else p = jrb_find_str(people, w2)->val.v;
		}
		else if (strcmp(w1, "MOTHER_OF") == 0){
			node = jrb_find_str(people, w2);
			if (node == NULL) c = CreatePerson(people, w2);
			else c = (struct person*) node->val.v;
			if (p->sex == 'M'){
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else p->sex = 'F';
			if (c->mother != NULL && c->mother != p){
				fprintf(stderr, "Bad input -- child with two mothers on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else if (c->mother != p){ //check if c is already a child of p
				dll_append(p->children, new_jval_v(c));
				c->mother = p;
			}
		}
		else if (strcmp(w1, "FATHER_OF") == 0){
			node = jrb_find_str(people, w2);
			if (node == NULL) c = CreatePerson(people, w2);
			else c = (struct person*) node->val.v;
			if (p->sex == 'F'){
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else p->sex = 'M';
			if (c->father != NULL && c->father != p){
				fprintf(stderr, "Bad input -- child with two fathers on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else if (c->father != p){ //check if c is already a child of p
				dll_append(p->children, new_jval_v(c));
				c->father = p;
			}
		}
		else if (strcmp(w1, "SEX") == 0){
			if (p->sex == 'U') p->sex = w2[0];
			else if (p->sex != w2[0]){
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
		}
		else if (strcmp(w1, "MOTHER") == 0){
			node = jrb_find_str(people, w2);
			if (node == NULL) c = CreatePerson(people, w2);
			else c = (struct person*) node->val.v;
			if (c->sex == 'M'){
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else c->sex = 'F';
			if (p->mother != NULL && p->mother != p){
				fprintf(stderr, "Bad input -- child with two mothers on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else if (p->mother != c){ //check if p is already a child of c
				dll_append(c->children, new_jval_v(p));
				p->mother = c;
			}
		}
		else if (strcmp(w1, "FATHER") == 0){
			node = jrb_find_str(people, w2);
			if (node == NULL) c = CreatePerson(people, w2);
			else c = (struct person*) node->val.v;
			if (c->sex == 'F'){
				fprintf(stderr, "Bad input - sex mismatch on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else c->sex = 'M';
			if (p->father != NULL && p->father != p){
				fprintf(stderr, "Bad input -- child with two fathers on line %d\n", input->line);
				jettison_inputstruct(input);
				free(w2);
				Genocide(people);
				exit(0);
			}
			else if (p->father != c){ //check if p is already a child of c
				dll_append(c->children, new_jval_v(p));
				p->father = c;
			}
		}
	}
	jettison_inputstruct(input);
	free(w2);
	/* check for cycles */
	jrb_traverse(node, people){
		p = (struct person*) node->val.v;
		if (DFS(p)){
			fprintf(stderr, "Bad input -- cycle in specification\n");
			Genocide(people);
			exit(0);
		}
	}
	/* print people */
	toprint = new_dllist();
	//put all of people on toprint
	jrb_traverse(node, people){
		p = (struct person*) node->val.v;
		p->visited = 0;
		dll_append(toprint, node->val);
	}
	PrintGraph(toprint);
	free_dllist(toprint);
	Genocide(people);
	return 0;
}
