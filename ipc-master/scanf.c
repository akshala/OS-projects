#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

int fd[2];

void first(){
	int x;
	char line[4096];
	x = fgets(line, sizeof(line), stdin);
	printf("Child: read [%s]\n", line);
}

void second(){
	do{
		int x, y, c;
		c = 0;
		char line[4096];
		x = fgets(line, sizeof(line), stdin);
		while (line[c] != '\0'){
			c++;	
		}
		line[c] = '\0';
		y = atoi(line);
		printf("Child: read [%d]\n", y);
	}while(1);
}

int main(){
	pipe(fd);
	pid_t pid = fork();

	if(pid == 0){
		// first();
	}
	else{
		second();
	}
}