//Jacob Vargo
//cs 360, lab 5
//Description:
// this prgram has four options: c, cv, x, xv. There are inputed using the first command line argument after the command name.
// c and cv:
//		v specifies that the files being tarred will be noted on stderr.
//		These options require at least one more command line argument that is the name of the file of directory to be tarred.
//		All spceicified files and directories and files reachable from those directories will be included into the jtar file.
//		The jtar file will be outputted to stdout.
// x and xv:
//		v specifies that the files being untarred will be noted on stderr.
//		A jtar file shall be read in from stdin.
//		All files and directories and files reachable from those directories will be recreated exactly as they were from the jtar file.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "dllist.h"
#include "jval.h"
#include "jrb.h"

int process_files(char *filename, JRB hard, JRB path, int print){
	struct stat buf;
	int status, i;
	DIR *d;
	struct dirent *de;
	char *str;
	Dllist dll_ptr, lis;
	JRB jrb_ptr;

	status = lstat(filename, &buf);
	if (status == -1){
		perror(filename);
		return 1;
	}
	if (S_ISDIR(buf.st_mode)){
		if (print) fprintf(stderr, "Directory %s %d\n", filename, buf.st_size);
		printf("%s\n", filename);
		fwrite(&buf, sizeof(struct stat), 1, stdout); //returns number of bytes written
		str = (char *) malloc(sizeof(char)*(strlen(filename)+258));
		lis = new_dllist();

		//traverse through directory contents and add the contents to lis
		d = opendir(filename);
		if (d == NULL){
			fprintf(stderr, "Couldn't open \"%s\"\n", filename);
			return 1;
		}
		for (de = readdir(d); de != NULL; de = readdir(d)){
			sprintf(str, "%s/%s", filename, de->d_name);
			status = lstat(str, &buf);
			if (status == -1){
				perror(str);
			}
			else if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){
				//fprintf(stderr, "appending: %s\n", str);
				dll_append(lis, new_jval_s(strdup(str)));
			}
		}
		closedir(d);
		free(str);
		str = NULL;

		dll_traverse(dll_ptr, lis){
			//identify duplicates and hard-links
			status = lstat(dll_ptr->val.s, &buf);
			if (status == -1){
				perror(filename);
				return 1;
			}
			//fprintf(stderr, "name: %s ino: %ul\n", dll_ptr->val.s, buf.st_ino);
			if (jrb_find_dbl(hard, (double)buf.st_ino) == NULL){
				//if (print) fprintf(stderr, "hard adding: %s\n", dll_ptr->val.s);
				jrb_insert_dbl(hard, (double)buf.st_ino, new_jval_s(strdup(dll_ptr->val.s)));
			}
			else{
				//hard link found. this inode is already on the tree
				if (print) fprintf(stderr, "(link %s to %s)\n", dll_ptr->val.s, "...");
			}
			str = realpath(dll_ptr->val.s, NULL);
			if (str == NULL){
				fprintf(stderr, "realpath() failed on %s\n", dll_ptr->val.s);
				return 1;
			}
			//fprintf(stderr, "real: %s\n", str);
			if (jrb_find_str(path, str) == NULL){
				//if (print) fprintf(stderr, "path adding: %s\n", dll_ptr->val.s);
				jrb_insert_str(path, str, new_jval_s(strdup(dll_ptr->val.s)));
			}
			else{
				//duplicate found. this path is already in the tree
				if (print) fprintf(stderr, "Duplicate found: %s\n", dll_ptr->val.s);
				continue;
			}
			i = process_files(dll_ptr->val.s, hard, path, print);
			if (i == 1){
				fprintf(stderr, "Error found\n");
				return i;
			}
		}
		dll_traverse(dll_ptr, lis) free(dll_ptr->val.s);
		free_dllist(lis);
		return 0;
	}
	else if (S_ISREG(buf.st_mode)){
		if (print) fprintf(stderr, "Regular File %s %d\n", filename, buf.st_size);
		printf("%s\n", filename);
		fwrite(&buf, sizeof(struct stat), 1, stdout); //returns number of bytes written
		return 0;
	}
	else if (S_ISLNK(buf.st_mode)){
		if (print) fprintf(stderr, "Ignoring soft link: %s\n", filename);
		return 0;
	}
	else{
		if (print) fprintf(stderr, "Unknown file type");
		return 0;
	}
}

int main(int argc, char **argv){
	int i, print;
	Dllist dll_ptr;
	JRB hard, path, jrb_ptr;
	struct stat buf;
	char *str;
	FILE * f;

	print = 0;
	if (argc >= 2){
		if(strcmp(argv[1], "c") == 0 || strcmp(argv[1], "cv") == 0){
			if (strcmp(argv[1], "cv") == 0) print = 1;
			if (argc >= 3){
				hard = make_jrb();
				path = make_jrb();
				for (i = 2; argc >= i+1; i++){
					process_files(argv[i], hard, path, print);
				}
				jrb_traverse(jrb_ptr, path){
					free(jrb_ptr->key.s);
					free(jrb_ptr->val.s);
				}
				jrb_traverse(jrb_ptr, hard){
					free(jrb_ptr->val.s);
				}
				jrb_free_tree(hard);
				jrb_free_tree(path);
			}
			else{
				fprintf(stderr, "Error: Could not find second argument\n");
				return 0;
			}
		}
		else if(strcmp(argv[1], "x") == 0 || strcmp(argv[1], "xv") == 0){
			if (strcmp(argv[1], "xv") == 0) print = 1;
			if (argc > 2){
				fprintf(stderr, "Error: Too many arguments\n");
				return 0;
			}

			//read tarfile from stdin
			str = (char *) malloc(sizeof(char) * 258);
			hard = make_jrb();
			path = make_jrb();
			while (1){
				i = 0; //being used to mark if what is being read is a hard link
				scanf("%s\n", str);
				if (feof(stdin)) break;
				if (print) fprintf(stderr, "File: %s\n", str);
				fread(&buf, sizeof(struct stat), 1, stdin);
				jrb_ptr = jrb_find_dbl(hard, (double)buf.st_ino);
				if (jrb_ptr == NULL){
					jrb_insert_dbl(hard, (double)buf.st_ino, new_jval_s(strdup(str)));
				}
				else{
					//hard link being read
					i = 1;
				}

				if (S_ISDIR(buf.st_mode)){
					if (mkdir(str, 0777) == 0){
						//put dir name on path tree
						jrb_ptr = jrb_find_int(path, buf.st_mode);
						if (jrb_ptr == NULL){
							jrb_insert_int(path, buf.st_mode, new_jval_s(strdup(str)));
						}
					}
					else{
						perror(str);
						//return 0;
					}
				}
				else if (S_ISREG(buf.st_mode)){
					if (i){ //make hard link
						if (link(jrb_ptr->val.s, str) == 0){
							if (print) fprintf(stderr, "(link %s to %s)\n", str, jrb_ptr->val.s);
						}
						else{
							perror(str);
						}
					}
					else{
						if (mknod(str, buf.st_mode, buf.st_dev) == 0){
							//store file contents.
							utime(str, buf.st_atime);
							chmod(str, buf.st_mode);
						}
						else{
							perror(str);
						}
					}
				}
				else{
					if (print) fprintf(stderr, "Error: Unknown file type");
				}
			}
			//traverse path tree and set dir protections
			free(str);
			jrb_traverse(jrb_ptr, path){
				free(jrb_ptr->val.s);
			}
			jrb_traverse(jrb_ptr, hard){
				free(jrb_ptr->val.s);
			}
			jrb_free_tree(hard);
			jrb_free_tree(path);
		}
		else{
			fprintf(stderr, "Error: Unrecognized option\n");
			return 0;
		}
	}
	else{
		fprintf(stderr, "Error: Not enough arguments\n");
		return 0;
	}
}
