#include <stdio.h> 
#include <stdlib.h> 
#include <sys/wait.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h> 

int flag_pipe = 0;
#define buffer_size 1000

char* get_command(char *command){
	int n = strlen(command);
	int index = -1;
	for(int i=n-1; i>=0; i--){
		if(command[i] == '/'){
			index = i;
			break;
		}
	}
	char *desired_command = (char*)(malloc(sizeof(char)*n));
	strcpy(desired_command,command+index+1);
	// printf("%s", desired_command);
	return desired_command;
}

char* add_space(char *without_space){
	int input_len = strlen(without_space);
	char *with_space = (char*)(malloc(sizeof(char)*input_len*3));
	for(int i=0,j=0; i<input_len; i++,j++){
		with_space[j] = without_space[i];
		if (without_space[i] == '|'){
			with_space[j++] = ' ';
			with_space[j++] = '|';
			with_space[j] = ' ';
		}
		else if (without_space[i] == '<'){
			with_space[j++] = ' ';
			with_space[j++] = '<';
			with_space[j] = ' ';
		}
		else if (without_space[i] == '1' && without_space[i+1] == '>'){
			with_space[j++] = ' ';
			with_space[j++] = '1';
			with_space[j++] = '>';
			with_space[j] = ' ';
			i++;
		}
		else if (without_space[i] == '2' && without_space[i+1] == '>' && without_space[i+2] != '&'){
			with_space[j++] = ' ';
			with_space[j++] = '2';
			with_space[j++] = '>';
			with_space[j] = ' ';
			i++;
		}
		else if (without_space[i] == '>' && without_space[i+1] == '>'){
			with_space[j++] = ' ';
			with_space[j++] = '>';
			with_space[j++] = '>';
			with_space[j] = ' ';
			i++;
		}
		else if (without_space[i] == '>' && without_space[i+1] != '&'){
			with_space[j++] = ' ';
			with_space[j++] = '>';
			with_space[j] = ' ';
		}
	}
	free(without_space);
	return with_space;
}

void remove_backslash_n(char *command_token){
	int len = strlen(command_token);
	if(command_token[len-1] == '\n'){
		command_token[len-1] = '\0';
	}
}

void redirect_stdout_to_file(char* file){
	close(1);
	creat(file, 0666);
}

void redirect_stderr_to_file(char* file){
	close(2);
	creat(file, 0666);
}

void redirect_append_to_file(char *file){
	close(1);
	open(file, O_WRONLY | O_CREAT | O_APPEND);
}

void redirect_stderr_to_stdout(char *file){
	close(2);
	dup(1);
}

void read_from_file(char *file){
	close(0);
	open(file, O_RDONLY);
}

int normal_exec(char *piping_commands[]){
	// pid = fork();
	piping_commands[0] = get_command(piping_commands[0]);
	int first_redirect_index = 0;
	for(int i=0; i<100 && piping_commands[i]!=NULL; i++){
		// printf("<%s>\n",piping_commands[i]);
		if(strcmp(piping_commands[i],"exit") == 0){
			// printf("exiting\n");
			exit(0);
		}
		if(strcmp(piping_commands[i],">") == 0 || strcmp(piping_commands[i],"1>") == 0){
			char* file = piping_commands[i+1];
			redirect_stdout_to_file(file);
			if (first_redirect_index == 0)
			{
				first_redirect_index = i;
			}
		}
		if(strcmp(piping_commands[i],">>") == 0){
			char* file = piping_commands[i+1];
			redirect_append_to_file(file);
			if (first_redirect_index == 0)
			{
				first_redirect_index = i;
			}
		}
		if(strcmp(piping_commands[i],"2>") == 0){
			char* file = piping_commands[i+1];
			redirect_stderr_to_file(file);
			if (first_redirect_index == 0)
			{
				first_redirect_index = i;
			}
		}
		if(strcmp(piping_commands[i],"2>&1") == 0){
			char* file = piping_commands[i+1];
			redirect_stderr_to_stdout(file);
			if (first_redirect_index == 0)
			{
				first_redirect_index = i;
			}
		}
		if(strcmp(piping_commands[i],"<") == 0){
			char* file = piping_commands[i+1];
			read_from_file(file);
			if (first_redirect_index == 0)
			{
				first_redirect_index = i;
			}
		}
	}
	if (first_redirect_index != 0)
	{
		piping_commands[first_redirect_index] = NULL;
	}

	execvp(piping_commands[0], piping_commands);
}

void rec_execute_pipe(char *piping_commands[][100]){
	if (piping_commands[1][0] == NULL){
		normal_exec(piping_commands[0]);
		return;	
	}
	if (piping_commands[2][0] == NULL) {
		int fd2[2];
		pipe(fd2);
		pid_t pid2 = fork();
		if (pid2 == 0) {
			// char *param[2] = {"/usr/bin/wc", NULL};
			close(fd2[0]);
			close(1);
			dup(fd2[1]);
			close(fd2[1]);
			// execvp(piping_commands[0][0], piping_commands[0]);
			normal_exec(piping_commands[0]);
		} 
		else {
			// char *param[2] = {"/bin/cat", NULL};
			close(fd2[1]);
			close(0);
			dup(fd2[0]);
			close(fd2[0]);
			// execvp(piping_commands[1][0], piping_commands[1]);
			normal_exec(piping_commands[1]);
		}
		return;
	}
	int fd1[2];
	pipe(fd1);
	pid_t pid1 = fork();
	if (pid1 == 0) {
		close(fd1[0]);
		close(1);
		dup(fd1[1]);
		close(fd1[1]);
		execvp(get_command(piping_commands[0][0]), piping_commands[0]);
	} 
	else {
		close(fd1[1]);
		close(0);
		dup(fd1[0]);
		close(fd1[0]);
		rec_execute_pipe((char * (*)[100])piping_commands[1]);
	}
}

int get_input(char *piping_commands[][100]) {
	int read_size;
	int arg_count = 0;
	int pipe_count = 0;
	char *args[100];
	
	char *input = (char*)malloc(sizeof(char*)*buffer_size);
	read_size = read(0, input, buffer_size);
	remove_backslash_n(input);
	input = add_space(input);
	// printf("%s",input);
	// exit(0);
	char *command_token = strtok(input, " ");
	 
	while (command_token != NULL) { 
	    // printf("<%s>\n", command_token);
	    args[arg_count] = command_token;
	    command_token = strtok(NULL, " "); 
	    arg_count++;
	}
	for (int i = 0; i < arg_count; i++) {
		// printf("<%s>\n", args[i]);
		int pipe_args = 0;
		while (i < arg_count && strcmp(args[i],"|")) {
			piping_commands[pipe_count][pipe_args] = args[i];
			pipe_args++;
			i++;
		}
		pipe_count++;
	}
	return pipe_count;
}

int main(){
	while(1){
		write(1, "$ ", 2);
		pid_t pid = fork();
		if(pid == 0){
			char *piping_commands[100][100];
			int pipe_count = get_input(piping_commands);
			// printf("%s\n", piping_commands[0][0]);
			if(strcmp(piping_commands[0][0], "exit") == 0){
				exit(1);
			}
			rec_execute_pipe(piping_commands);
		}
		else if(pid > 0){
			int status;
			wait(&status);
			if (status != 0){
				break;
			}
		}
	}
}