#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

// you can change the prototype of existing
// functions, add new routines, and global variables
// cheatmode, car1, car2, report are different processes
// they communicate with each other via pipes

int fd_cheat_car1[2];
int fd_cheat_car2[2];
int fd_cheat_car1_pos[2];
int fd_cheat_car2_pos[2];
int fd_report_car1[2];
int fd_report_car2[2];
int fd_car1_pos[2];
int fd_car2_pos[2];

// step-1
// ask user's if they want to cheat
// if yes, ask them if they want to relocate car1 or car2
// ask them the new location of car1/car2
// instruct car1/car2 to relocate to new position
// goto step-1
void cheatmode()
{
	do{
		int x;
		char *a;
		int answer, z; 
		char line1[4096], line2[4096], line3[4096];

		printf("Would you like to cheat? Enter 1 for yes and 0 for no.\n");
		a = fgets(line1, sizeof(line1), stdin);
		int c1 = 0;
		while (line1[c1] != '\0'){
			c1++;	
		}
		line1[c1] = '\0';
		answer = atoi(line1);

		if(answer == 1){
			int car_to_be_relocated, relocated_location, x, cheat, y;
			cheat = 1;
			printf("Would you like to relocate car1 or car2? Enter 1 or 2\n");
			a = fgets(line2, sizeof(line2), stdin);
			int c2 = 0;
			while (line2[c2] != '\0'){
				c2++;	
			}
			line2[c2] = '\0';
			car_to_be_relocated = atoi(line2);
			printf("relocated car %d\n", car_to_be_relocated);

			printf("Enter the new location of %d\n", car_to_be_relocated);
			a = fgets(line3, sizeof(line3), stdin);
			int c3 = 0;
			while (line3[c3] != '\0'){
				c3++;	
			}
			line3[c3] = '\0';
			relocated_location = atoi(line3);

			if(car_to_be_relocated == 1){
				x = write(fd_cheat_car1_pos[1], &relocated_location, sizeof(relocated_location));
				x = write(fd_cheat_car1[1], &cheat, sizeof(cheat));
			}
			if(car_to_be_relocated == 2){
				x = write(fd_cheat_car2_pos[1], &relocated_location, sizeof(relocated_location));
				x = write(fd_cheat_car2[1], &cheat, sizeof(cheat));
			}
		}
	}while(1);
}

// step-1
// check if report wants me to terminate
// if yes, terminate
// sleep for a second
// generate a random number r between 0-10
// advance the current position by r steps
// check if cheat mode relocated me
// if yes set the current position to the new position
// send the current postion to report
// make sure that car1 and car2 generates a different
// random number
// goto step-1
void car1()
{
	int car1_pos = 0;
	srand(1);
	do{
		int report_status, x, r1, cheat;
		fcntl(fd_report_car1[0], F_SETFL, O_NONBLOCK);
		x = read(fd_report_car1[0], &report_status, sizeof(report_status));
		if(x == -1){
			report_status = 0;
		}
		if(report_status == 1){
			exit(0);
			return;
		}
		sleep(1);
		r1 = rand() % 11;
		car1_pos += r1;
		fcntl(fd_cheat_car1[0], F_SETFL, O_NONBLOCK);
		x = read(fd_cheat_car1[0], &cheat, sizeof(cheat));
		if(x == -1){
			cheat = 0;
		}
		if(cheat == 1){
			x = read(fd_cheat_car1_pos[0], &car1_pos, sizeof(car1_pos));
		}
		x = write(fd_car1_pos[1], &car1_pos, sizeof(car1_pos));
	}while(1);
}

// step-1
// check if report wants me to terminate
// if yes, terminate
// sleep for a second
// generate a random number r between 0-10
// advance the current position by r steps
// check if cheat mode relocated me
// if yes set the current position to the new position
// send the current postion to report
// make sure that car1 and car2 generates a different
// random number
// goto step-1
void car2()
{
	int car2_pos = 0;
	srand(2);
	do{
		int report_status, x, r2, cheat;
		fcntl(fd_report_car2[0], F_SETFL, O_NONBLOCK);
		x = read(fd_report_car2[0], &report_status, sizeof(report_status));
		if(x == -1){
			report_status = 0;
		}
		if(report_status == 1){
			exit(0);
			return;
		}
		sleep(1);
		r2 = rand() % 11;
		car2_pos += r2;
		fcntl(fd_cheat_car2[0], F_SETFL, O_NONBLOCK);
		x = read(fd_cheat_car2[0], &cheat, sizeof(cheat));
		if(x == -1){
			cheat = 0;
		}
		if(cheat == 1){
			x = read(fd_cheat_car2_pos[0], &car2_pos, sizeof(car2_pos));
		}
		x = write(fd_car2_pos[1], &car2_pos, sizeof(car2_pos));
	}while(1);
}

// step-1
// sleep for a second
// read the status of car1
// read the status of car2
// whoever completes a distance of 100 steps is decalared as winner
// if both cars complete 100 steps together then the match is tied
// if (any of the cars have completed 100 steps)
//    print the result of the match
//    ask cars to terminate themselves
//    kill cheatmode using kill system call
//    return to main if report is the main process
// else
// 	  print the status of car1 and car2
// goto step-1
void report()
{
	do{
		sleep(1);
		int car1_pos, car2_pos, x, report1, report2;
		x = read(fd_car1_pos[0], &car1_pos, sizeof(car1_pos));
		x = read(fd_car2_pos[0], &car2_pos, sizeof(car2_pos));

		if(car1_pos >= 100 && car2_pos >= 100){
			printf("Tied!\n");
			report1 = 1;
			report2 = 1;
			x = write(fd_report_car1[1], &report1, sizeof(report1));
			x = write(fd_report_car2[1], &report2, sizeof(report2));
			pid_t pid = getppid();
			kill(pid, SIGKILL);
			exit(0);
			return;
		}

		else if(car1_pos >= 100){
			printf("Car1 wins\n");
			report1 = 1;
			report2 = 1;
			x = write(fd_report_car1[1], &report1, sizeof(report1));
			x = write(fd_report_car2[1], &report2, sizeof(report2));
			pid_t pid = getppid();
			kill(pid, SIGKILL);
			exit(0);
			return;
		}

		else if(car2_pos >= 100){
			printf("Car2 wins\n");
			report1 = 1;
			report2 = 1;
			x = write(fd_report_car1[1], &report1, sizeof(report1));
			x = write(fd_report_car2[1], &report2, sizeof(report2));
			pid_t pid = getppid();
			kill(pid, SIGKILL);
			exit(0);
			return;
		}
		else{
			printf("Car1 %d\n", car1_pos);
			printf("Car2 %d\n", car2_pos);
		}
	}while(1);
		
}

// create pipes
// create all processes
// wait for all processes to terminate
int main()
{
	int cheat_car1 = pipe(fd_cheat_car1);
	int cheat_car2 = pipe(fd_cheat_car2);
	int report_car1 = pipe(fd_report_car1);
	int report_car2 = pipe(fd_report_car2);
	int car1_pos = pipe(fd_car1_pos);
	int car2_pos = pipe(fd_car2_pos);
	int cheat_car1_pos = pipe(fd_cheat_car1_pos);
	int cheat_car2_pos = pipe(fd_cheat_car2_pos);

	pid_t pid1 = fork();
	pid_t pid2 = fork();

	if(pid1 > 0 && pid2 > 0){
		cheatmode();
	}
	if(pid1 == 0 && pid2 > 0){
		report();
	}
	if(pid1 > 0 && pid2 == 0){
		car1();
	}
	else{
		car2();
	}
	return 0;
}