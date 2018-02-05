//Jacob Vargo
//cs 360, lab2
//Description:
//Read a compressed input from stdin, decipher it and print it out

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#define BUF_SIZE 256

int main(int argc, char **argv){
	FILE *f;
	char first;
	int n, size, i;
	int needSpace; //true when a space needs to be printed before the next sequence is printed
	int bufi[BUF_SIZE];
	double bufd[BUF_SIZE];
	unsigned char bufs[BUF_SIZE+1]; //+1 to make room for null terminating char

	f = stdin;
	first = getchar();
	needSpace = 0;
	while (first != EOF){
		if (first == 'n'){
			printf("\n");
			needSpace = 0;
		}
		else{
			n = getchar();
			if (n == EOF && (first == 'i' || first == 'd' || first == 's')){
				fprintf(stderr, "Input error: no size\n");
				exit(0);
			}
			else if (first == 'i'){
				//read n+1 ints. ints are 4 chars long
				if (fread(bufi, 4, n+1, f) != n+1){
					fprintf(stderr, "Input error: not enough ints\n");
					exit(0);
				}
				if (needSpace){
					printf(" ");
					needSpace = 0;
				}
				for (i = 0; i < n+1; i++){
					if (i != 0) printf(" ");
					printf("%d", bufi[i]);
				}
			}
			else if (first == 'd'){
				//read n+1 doubles. doubles are 8 bytes long
				if (fread(bufd, 8, n+1, f) != n+1){
					fprintf(stderr, "Input error: not enough doubles\n");
					exit(0);
				}
				if (needSpace){
					printf(" ");
					needSpace = 0;
				}
				for (i = 0; i < n+1; i++){
					if (i != 0) printf(" ");
					printf("%.10lg", bufd[i]);
				}
			}
			else if (first == 's'){
				//read n+1 c-strings
				for (i = 0; i < n+1; i++){
					size = getchar();
					if (size == EOF){
						fprintf(stderr, "Input error: no string size\n");
						exit(0);
					}
					//read size+1 chars into bufs. chars are 1 byte long
					if (fread(bufs, 1, size+1, f) != 1*(size+1)){
						fprintf(stderr, "Input error: not enough chars\n");
						exit(0);
					}
					bufs[size+1] = '\0'; //null terminate bufs
					if (needSpace){
						printf(" ");
						needSpace = 0;
					}
					if (i != 0) printf(" ");
					printf("%s", bufs);
				}
			}
			else{
				fprintf(stderr, "Input error: bad type\n");
				exit(0);
			}
		}
		if (first != 'n') needSpace = 1;
		first = getchar();
	}
	return 0;
}
