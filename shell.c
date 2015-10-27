/*Heavily referenced from http://stephen-brennan.com/2015/01/16/write-a-shell-in-c/
 * in my_system*/

#define _XOPEN_SOURCE 700 

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

char** get_args(char *old_arg_array, int *argc) {
	char **args, *temp_token;
	int index = 0, i, k = 0;

	/*allocate memory with buf size of 20*/
	args = malloc(sizeof(char*) * 20);

	/*get first 'token'*/
	temp_token = strtok(old_arg_array, " ,");

	/*while token exists*/
	while(temp_token != NULL) {
		/*store in new array, increment index*/
		args[index] = temp_token;
		index++;

		/*get next token*/
		temp_token = strtok(0, ", ");
	}

	/*end of array is a null character*/
	args[index] = 0;

	*argc = index;

	/*there are newlines in here that mess up
	 * strcmp, so i'm removing them.*/
	for (i = 0; i < index; i++) {
		while (args[i][k] != 0) {
			if (args[i][k] == '\n') {
				args[i][k] = 0;
				break;
			}

			k++;
		}

		k = 0;
	}

	/*return array*/
	return args;

}

int is_redir(char **args, int argc) {
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp(args[i], "<"))
			return i;
	}

	return 0;
}

int my_system(char **args, int argc) {
	int x;
	pid_t pid, waiting;

	pid = fork();

	switch (pid) {
	case 0:
		if(execvp(args[0], args) == -1) {
			return 0;
		}

	case -1:
		return 0;

	default:
		do {
			waiting = waitpid(pid, &x, WUNTRACED);
		} while (!WIFEXITED(x) && !WIFSIGNALED(x));
	}

	return 1;;
}

int is_pipe(char **args, int argc) {
	int i;

	for (i = 0; i < argc; i++) {
		if (!strcmp(args[i], "|"))
			return i;
	}

	return 0;
}

int actual_pipe(char **first, char **second) {
	int fd[2];
   
	pipe(fd);
      
    	if (fork() == 0) {
      		dup2(fd[0], STDIN_FILENO);
                close(fd[1]);
		close(fd[0]);
      		if (fork() == 0) { 
     			dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                        close(fd[1]);
                        execvp(first[0], first);
                }

      		wait(0);
                execvp(second[0], second);
      }

      close(fd[1]);
      close(fd[0]);
      wait(0);

      return 1;
}

int my_pipe(char **args, int argc, int index) {
	char **first_cmd, **second_cmd;
	int i, k = 0;

	first_cmd = malloc(sizeof(char *) * 20);
	second_cmd = malloc(sizeof(char *) * 20);

	for (i = 0; i < index; i++) {
		first_cmd[i] = args[i];
	}

	first_cmd[index] = 0;

	for (i = index + 1; i < argc; i++) {
		second_cmd[k] = args[i];
		k++;
	}

	second_cmd[k] = 0;

	if(!actual_pipe(first_cmd, second_cmd)) {
		printf("Error piping.\n");
		return 0;
	}

	free(first_cmd);
	free(second_cmd);

	return 1;
}

int main() {

	while (1) {
		int num = 0, x, std_in = dup(STDIN_FILENO), std_out = dup(STDOUT_FILENO), index;
		char *orig_args = 0, **new_args;
		ssize_t getline_buffer = 0;
	
		printf(">> ");
		getline(&orig_args, &getline_buffer, stdin);

		new_args = get_args(orig_args, &num);

		index = is_redir(new_args, num);

		if (!strcmp(new_args[0], "exit")) {
			free(new_args);
			free(orig_args);
			return 0;
		}

		else if (!strcmp(new_args[0], "cd")) {
			if (!new_args[1]) {
				chdir(getenv("HOME"));
			}
			
			else {
				x = chdir(new_args[1]);
				if (x) {
					printf(">> : No directory by the name of '%s' exists.\n", new_args[1]);
				}	
			}
		}
		
		else if ((num >= 3) && index) {
			x = open(new_args[index + 1], O_WRONLY | O_CREAT, 0666);

			/*this is pretty much exactly what the in class example
			 * redirect.c is*/
			if (x != -1) {
				dup2(x, STDOUT_FILENO);
				close(x);

				/*we made stdout go to the specified file, so now we 
				 * just need to focus on the command given.*/
				new_args[index] = 0;

				x = my_system(new_args, num);
				
				dup2(std_out, STDOUT_FILENO);
				dup2(std_in, STDIN_FILENO);

				close(std_in);
				close(std_out);

				if (!x) {
					printf(">> : Could not execute command(s).\n");
				}
			}

			else {
				printf(">> : No file by the name of '%s' exists.\n", new_args[2]);
			}

		}

		else if (index = is_pipe(new_args, num)) {
			if (!my_pipe(new_args, num, index)) {
				printf("Could not pipe.\n");
			}
		}

		else {
			x = my_system(new_args, num);

			if(!x) {
				printf(">> : Could not execute command(s).\n");
			}
		}

		free(new_args);
		free(orig_args);
	}

	/*will never happen, but here just in case.*/
	return 0;

}
