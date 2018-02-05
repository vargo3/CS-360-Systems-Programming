//Jacob Vargo
//cs360 labA
//Description:
//This program is a general talk server
//usage: chat-server Port Chat-Room-Names ...
//This program setups a server with multiple chat rooms enumerated and named by Chat-Room-Names ...
//A user may connenct to the machine the server is launched on with the port number given by Port
//Once connected a user is asked for their chat name and which room to enter.
//All users registered within a chat room will recieve messages from all other user is the same room. Users in other rooms will not recieve the same messages.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sockettome.h"
#define STR_SIZE 1000
#define PRINT 1

struct list_node{
	char *str;
	FILE *fout;
	struct list_node *flink;
	struct list_node *blink;
};

struct Chat_Room{
	char *name;
	struct list_node *user_head; //head of doubly-linked-list of clients
	struct list_node *user_tail; //tail of doubly-linked-list of clients
	struct list_node *output; //head of singly-linked-list of strings to print to all users in user list
	pthread_mutex_t user_lock; //mutex for user list
	pthread_mutex_t output_lock; //mutex for output list
	pthread_cond_t output_cond; //condition variable that signals that output list has been appended
};

struct Client{
	int fd;
	int num_rooms;
	struct Chat_Room *rooms;
};

void *client_thread(void *arg){
	char buf[STR_SIZE], output_line[STR_SIZE], *name;
	FILE *fsock[2];
	struct Client *me;
	struct Chat_Room *room;
	struct list_node *node;
	int i, buflen;

	me = (struct Client*) arg;
	fsock[0] = fdopen(me->fd, "r");
	fsock[1] = fdopen(me->fd, "w");

	fflush(fsock[1]);
	sprintf(buf, "Chat Rooms:\n\n");
	if (fputs(buf, fsock[1]) == EOF || fflush(fsock[1]) == EOF){
		fclose(fsock[0]);
		fclose(fsock[1]);
		if (PRINT) printf("Client_thread dies\n");
		pthread_exit(NULL);
	}
	for (i = 0; i < me->num_rooms; i++){
		sprintf(buf, "%s:", me->rooms[i].name);
		//add to buf the names of users in room ordered by first to enter
		pthread_mutex_lock(&(me->rooms[i].user_lock));
		node = me->rooms[i].user_head;
		buflen = strlen(buf);
		while (node != NULL && buflen < STR_SIZE){
			strcat(buf, " ");
			strcat(buf, node->str);
			buflen += strlen(node->str)+1;
			node = node->flink;
		}
		strcat(buf, "\n");
		pthread_mutex_unlock(&(me->rooms[i].user_lock));
		if (fputs(buf, fsock[1]) == EOF || fflush(fsock[1]) == EOF){
			fclose(fsock[0]);
			fclose(fsock[1]);
			if (PRINT) printf("Client_thread dies\n");
			pthread_exit(NULL);
		}
	}

	sprintf(buf, "\nEnter your chat name (no spaces):\n");
	if (fputs(buf, fsock[1]) == EOF || fflush(fsock[1]) == EOF){
		fclose(fsock[0]);
		fclose(fsock[1]);
		if (PRINT) printf("Client_thread dies\n");
		pthread_exit(NULL);
	}
	if (fgets(buf, STR_SIZE, fsock[0]) == NULL){
		fclose(fsock[0]);
		fclose(fsock[1]);
		if (PRINT) printf("Client_thread dies\n");
		pthread_exit(NULL);
	}
	buf[strlen(buf)-1] = '\0'; //overwrite '\n'
	name = strdup(buf);

	room = NULL;
	while (room == NULL){ //loop until valid room is given
		sprintf(buf, "Enter chat room:\n");
		if (fputs(buf, fsock[1]) == EOF || fflush(fsock[1]) == EOF){
			fclose(fsock[0]);
			fclose(fsock[1]);
			free(name);
			if (PRINT) printf("Client_thread dies\n");
			pthread_exit(NULL);
		}
		if (fgets(buf, STR_SIZE, fsock[0]) == NULL){
			fclose(fsock[0]);
			fclose(fsock[1]);
			free(name);
			if (PRINT) printf("Client_thread dies\n");
			pthread_exit(NULL);
		}
		buf[strlen(buf)-1] = '\0'; //overwrite '\n'
		//look for room with name given in buf
		for (i = 0; i < me->num_rooms; i++){
			if (strcmp(buf, me->rooms[i].name) == 0){
				room = &(me->rooms[i]);
				break;
			}
		}
		if (room == NULL){
			sprintf(buf, "Invalid chat room name.\n");
			if (fputs(buf, fsock[1]) == EOF || fflush(fsock[1]) == EOF){
				fclose(fsock[0]);
				fclose(fsock[1]);
				free(name);
				if (PRINT) printf("Client_thread dies\n");
				pthread_exit(NULL);
			}
		}
	}

	//add client to room
	pthread_mutex_lock(&(room->user_lock));
	if (room->user_tail == NULL){ //list is empty
		room->user_head = (struct list_node *) malloc(sizeof(struct list_node));
		room->user_tail = room->user_head;
		room->user_tail->blink = NULL;
	}
	else{
		node = (struct list_node *) malloc(sizeof(struct list_node));
		room->user_tail->flink = node;
		node->blink = room->user_tail;
		room->user_tail = node;
	}
	room->user_tail->flink = NULL;
	room->user_tail->fout = fsock[1];
	room->user_tail->str = strdup(name);
	pthread_mutex_unlock(&(room->user_lock));
	//put "user has joined" on chat room's list
	pthread_mutex_lock(&(room->output_lock));
	if (room->output == NULL){
		room->output = (struct list_node*) malloc(sizeof(struct list_node));
		node = room->output;
	}
	else{
		node = room->output;
		while (node->flink != NULL) node = node->flink;
		node->flink = (struct list_node*) malloc(sizeof(struct list_node));
		node = node->flink;
	}
	node->blink = NULL; //output list is being used as a singly-linked list; no blink needed
	node->flink = NULL;
	node->fout = NULL;
	node->str = NULL;
	sprintf(buf, "%s has joined\n", name);
	node->str = strdup(buf);
	pthread_cond_signal(&(room->output_cond));
	pthread_mutex_unlock(&(room->output_lock));

	//read and send user input to room until user disconnects
	while (fgets(buf, STR_SIZE, fsock[0]) != NULL){
		//put input string on chat room's list
		pthread_mutex_lock(&(room->output_lock));
		if (room->output == NULL){
			node = (struct list_node*) malloc(sizeof(struct list_node));
			room->output = node;
		}
		else{
			node = room->output;
			while (node->flink != NULL) node = node->flink;
			node->flink = (struct list_node*) malloc(sizeof(struct list_node));
			node = node->flink;
		}
		node->blink = NULL; //output list is being used as a singly-linked list; no blink needed
		node->flink = NULL;
		node->fout = NULL;
		node->str = NULL;
		sprintf(output_line, "%s: %s", name, buf);
		node->str = strdup(output_line);
		pthread_cond_signal(&(room->output_cond));
		pthread_mutex_unlock(&(room->output_lock));
		if (PRINT) printf("Read: %s", buf);
	}

	pthread_mutex_lock(&(room->output_lock));
	if (room->output == NULL){
		room->output = (struct list_node*) malloc(sizeof(struct list_node));
		node = room->output;
	}
	else{
		node = room->output;
		while (node->flink != NULL) node = node->flink;
		node->flink = (struct list_node*) malloc(sizeof(struct list_node));
		node = node->flink;
	}
	node->blink = NULL; //output list is being used as a singly-linked list; no blink needed
	node->flink = NULL;
	node->fout = NULL;
	node->str = NULL;
	sprintf(buf, "%s has left\n", name);
	node->str = strdup(buf);
	pthread_cond_signal(&(room->output_cond));
	pthread_mutex_unlock(&(room->output_lock));
	fclose(fsock[0]);
	//chat room will close fsock[1]
	free(name);
	if (PRINT) printf("Client_thread dies\n");
	pthread_exit(NULL);
}

void *chat_room_thread(void *arg){
	struct Chat_Room *me;
	struct list_node *user_node, *temp;

	me = (struct Chat_Room*) arg;
	pthread_mutex_lock(&(me->output_lock));
	while (1){
		pthread_cond_wait(&(me->output_cond), &(me->output_lock));
		if (PRINT) printf("%s has recieved a signal\n", me->name);

		//send each string on output list to each client in the room
		pthread_mutex_lock(&(me->user_lock));
		while (me->output != NULL){
			if (PRINT) printf("Sending %s to all users in %s\n", me->output->str, me->name);
			user_node = me->user_head;
			while (user_node != NULL){
				if (PRINT) printf("Sending %s to %s\n", me->output->str, user_node->str);
				if (fputs(me->output->str, user_node->fout) == EOF || fflush(user_node->fout) == EOF){
					//remove client from user list and close its fd
					//fflush(user_node->fout);
					if (user_node->blink != NULL)
						user_node->blink->flink = user_node->flink;
					else{ //user_node == me->user_head
						me->user_head = user_node->flink; //NULL if list will be emptied
						if (me->user_head != NULL) me->user_head->blink = NULL;
					}
					if (user_node->flink != NULL)
						user_node->flink->blink = user_node->blink;
					else{ //user_node == me->user_tail
						me->user_tail = user_node->blink; //NULL if list will be emptied
						if (me->user_tail != NULL) me->user_tail->flink = NULL;
					}

					//remove user's node
					if (PRINT) printf("%s has closed %s's_output\n", me->name, user_node->str);
					temp = user_node;
					user_node = user_node->flink;
					fclose(temp->fout);
					free(temp->str);
					free(temp);
				}
				else{
					user_node = user_node->flink;
				}
			}
			temp = me->output;
			me->output = me->output->flink;
			free(temp->str);
			free(temp);
		}
		pthread_mutex_unlock(&(me->user_lock));
	} //end of while(1) loop
	pthread_mutex_unlock(&(me->output_lock));

	//remove all clients from user list and close their fd's and free their strs
	pthread_mutex_lock(&(me->user_lock));
	user_node = me->user_head;
	while (user_node != NULL){
		temp = user_node;
		user_node = user_node->flink;
		fclose(temp->fout);
		free(temp->str);
		free(temp);
	}
	pthread_mutex_unlock(&(me->user_lock));

	//empty output list
	pthread_mutex_lock(&(me->output_lock));
	while (me->output != NULL){
		temp = me->output;
		me->output = me->output->flink;
		free(temp->str);
		free(temp);
	}
	pthread_mutex_unlock(&(me->output_lock));
	if (PRINT) printf("%s has died\n", me->name);
	pthread_exit(NULL);
}

int main(int argc, char **argv){
	int i, j, socket, port, num_rooms;
	pthread_t tid;
	void *ret;
	char *s;
	struct Client *p_client;
	struct Chat_Room *rooms;

	if (argc < 3){
		fprintf(stderr, "Usage: chat-server port Chat-Room-Names ...\n");
		return 0;
	}
	port = atoi(argv[1]);
	socket = serve_socket(port);

	//make list of chat rooms
	num_rooms = argc-2;
	rooms = (struct Chat_Room *) malloc(sizeof(struct Chat_Room) * num_rooms);
	for (i = 0; i < num_rooms; i++){
		rooms[i].name = argv[i+2];
		//order rooms by name lexicographically
		for (j = 0; j < i; j++){
			if (strcmp(rooms[j].name, rooms[i].name) == 0)
				break;
			else if (strcmp(rooms[j].name, rooms[i].name) > 0){
				s = rooms[j].name;
				rooms[j].name = rooms[i].name;
				rooms[i].name = s;
			}
		}
		rooms[i].user_head = NULL;
		rooms[i].user_tail = NULL;
		pthread_mutex_init(&(rooms[i].output_lock), NULL);
		pthread_mutex_init(&(rooms[i].user_lock), NULL);
		pthread_cond_init(&(rooms[i].output_cond), NULL);
		pthread_create(&tid, NULL, chat_room_thread, (void*) &(rooms[i]));
	}
	while(1){
		i = accept_connection(socket);
		if (PRINT) printf("Connection established\n");
		p_client = (struct Client *) malloc(sizeof(struct Client));
		p_client->fd = i;
		p_client->num_rooms = num_rooms;
		p_client->rooms = rooms;
		pthread_create(&tid, NULL, client_thread, (void*) p_client);
	}
	free(rooms);
	return 0;
}
