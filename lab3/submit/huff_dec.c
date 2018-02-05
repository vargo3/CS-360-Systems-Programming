//Jacob Vargo
//cs 360, lab 3
//Description:
//takes two command-line arguments: code-file input-file
//code-file is read and stored in a binary tree. 
//input-file is read and decrypted using the info stored from code file.
//in the binary tree: tranverse to left child if a 0 bit was read in sequence from input file. right child represents a 1 was read in sequence.
//when traversing tree, once a leaf node is found print is value and start from the top of tree

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define BUF_SIZE 10000

typedef struct node{
	struct node *left;
	struct node *right;
	char *val;
} *Node_ptr;

Node_ptr make_Node(){
	Node_ptr node;
	node = malloc(sizeof(struct node));
	node->left = NULL;
	node->right = NULL;
	node->val = NULL;
	return node;
}

void destroy_Tree(Node_ptr node){
	if (node == NULL) return;
	destroy_Tree(node->left);
	destroy_Tree(node->right);
	free(node->val);
	free(node);
}

int main(int argc, char **argv){
	FILE *file;
	Node_ptr head, node; //head is a binary tree
	char buf[BUF_SIZE+1];
	long int bufsize, inputsize;
	long int i, j;

	if (argc != 3){
		fprintf(stderr, "bad num args:\nusage: code definition file, input file\n");
		return(0);
	}

	//read in code file into a binary tree (head)
	file = fopen(argv[1], "r");
	if (file == NULL){
		perror("Error opening code file");
		return 0;
	}
	head = malloc(sizeof(struct node));
	while (!feof(file)){
		bufsize = 0; 
		while (bufsize < BUF_SIZE){
			i = fgetc(file);
			if (i == EOF && bufsize == 0) break;
			//Error check: code definition file is not in the correct format
			if (i == EOF && bufsize != 0){ //a string was still being read in and its value has not yet been given
				fprintf(stderr, "Error code file is not formatted correctly: %.*s\n", bufsize, buf);
				fclose(file);
				destroy_Tree(head);
				return 0;
			}
			buf[bufsize] = i;
			bufsize++;
			if (i == '\0') break; //allows null char to be added before breaking
		}
		//put buf string on the tree. construct tree as needed.
		node = head;
		while(!feof(file)){
			i = fgetc(file);
			if (i == '0'){
				if (node->left == NULL) node->left = make_Node();
				node = node->left;
			}
			else if (i == '1'){
				if (node->right == NULL) node->right = make_Node();
				node = node->right;
			}
			else if (i == '\0'){
				node->val = malloc(sizeof(char) * (bufsize+1));
				strcpy(node->val, buf);
				break;
			}
			else{
				//Error check: code definition file is not in the correct format
				fprintf(stderr, "Error code file is not formatted correctly: %d\n", i);
				fclose(file);
				destroy_Tree(head);
				return 0;
			}
		}
	}
	fclose(file);

	//check in input file
	file = fopen(argv[2], "r");
	if (file == NULL){
		perror("Error opening code file");
		destroy_Tree(head);
		return 0;
	}
	//read last 4 bytes
	fseek(file, 0, SEEK_END);
	i = ftell(file);
	if (i < 4){
		//Error check: Input file is less than 4 bytes
		fprintf(stderr, "Error: file is not the correct size.\n");
		destroy_Tree(head);
		fclose(file);
		return 0;
	}
	fseek(file, -4, SEEK_END);
	fread(&inputsize, 4, 1, file);
	if (ceil(inputsize/8.0)+4 != i){
		//Error check: size of input file does not match the specfied number of bits
		fprintf(stderr, "Error: Total bits = %ld, but file's size is %ld\n", inputsize, i);
		destroy_Tree(head);
		fclose(file);
		return 0;
	}
	//read in input file and find code in tree
	fseek(file, 0, SEEK_SET);
	node = head;
	while(1){
		i = fgetc(file);
		if (feof(file) || inputsize == 0) break;
		for (j = 0; j < 8 && inputsize != 0; j++){
			if (i & (1 << j)){ //read a 1 bit
				if (node->right != NULL) node = node->right;
				else{
					//Error check: unrecognized sequence of bits, or and imcomplete sequence at the end of the bitstream.
					fprintf(stderr, "Error unreconginized sequence of bits\n");
					destroy_Tree(head);
					fclose(file);
					return 0;
				}
			}
			else{ //read a 0 bit
				if (node->left != NULL) node = node->left;
				else{
					fprintf(stderr, "Error unreconginized sequence of bits\n");
					destroy_Tree(head);
					fclose(file);
					return 0;
				}
			}
			if (node->val != NULL){
				printf("%s", node->val);
				node = head;
			}
			inputsize--;
		}
	}
	fclose(file);
	destroy_Tree(head);
	return 0;
}
