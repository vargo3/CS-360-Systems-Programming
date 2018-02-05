//Jacob Vargo
//cs360 lab8
//Description:
//This is a bare bones shell program.
//When launched, this program takes one optional argument. If is the argument is given, it will be used as the shell prompt.
//This program will read a line from stdin. Then it will call fork() and have the child call exec() to execute the command that was read in.
//Optionally, if the character '&' is appended to the command, then the shell will not wait for that child to exit.
//The command "exit" will end the program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define STR_SIZE 1000

int main(int argc, char **argv){
	char *prompt, user_command[STR_SIZE], *com_v[STR_SIZE];
	int dummy, com_len, i, child_pid;

	if (argc == 1)
		prompt = strdup("jsh1: ");
	else if (argc == 2){
		if (strcmp(argv[1], "-") == 0)
			prompt = strdup("");
		else
			prompt = strdup(argv[1]);
	}
	else{
		fprintf(stderr, "Error: can only accept one command line argument\n");
		return;
	}

	while(1){
		user_command[0] = '\0'; //clear old command
		printf("%s", prompt);
		fgets(user_command, STR_SIZE, stdin);
		com_len = strlen(user_command)-1;
		user_command[com_len] = '\0'; //removing the newline char
		if (strcmp(user_command, "exit") == 0 || strcmp(user_command, "") == 0){
			break;
		}

		//Parse the user command into individual arguments
		com_v[0] = strtok(user_command, " ");
		for (i = 1; i < STR_SIZE; i++){
			com_v[i] = strtok(NULL, " ");
			if (com_v[i] == NULL)
				break;
			else if (strcmp(com_v[i], "&") == 0){
				com_v[i] = NULL;
				break;
			}
		}

		fflush(stdout);
		child_pid = fork();
		if (child_pid == 0){
			//child process
			free(prompt);
			execvp(*com_v, com_v);
			perror(*com_v);
			return 0; //kill child
		}
		if (user_command[com_len-1] != '&'){
			while (wait(&dummy) != child_pid); //wait until most recently forked child returns
		}
	}
	
	while(wait(&dummy) >= 0); //wait until all children are dead
	free(prompt);
	return 0;
}
