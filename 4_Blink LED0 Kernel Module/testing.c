/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 5/22/20
 *
 * testing: opens, reads and writes to "/sys/module/hw4/parameters/blink_rate" 
 * 		module parameter to view and modify the value of 
 * 		"blink_rate" in hw4 module. The program also opens "/dev/ece_led" first
 *		to start the timer.
 *
 *		Note: the user must enter a new value, otherwise blink_rate=2.
 *
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(int argc, char *argv[]){
	
	int size;
	char *val;			//to read current value of blink_rate
	char *new_val;		//new value to be assigned to blink_rate

	new_val = (char*)malloc(10*sizeof(char));

	if(argc<2){
		new_val = "2";	//default value if user didn't enter a value
	}
	else{
		if(atoi(argv[1])<=0){
			free(new_val);
			fprintf(stderr, "ERROR: Invalid Argument\n");
			exit(-1);
		}
		else{
			new_val = (argv[1]); //assigning new_val with a value entered by user
		}
	}

	/*opening the kernel module node file /dev/ece_led */
	int fd1 = open("/dev/ece_led", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open failed*/
	if(fd1<0){
		perror("ERROR: ");
		free(new_val);
		exit(1);
	}


	int fd2 = open("/sys/module/hw4/parameters/blink_rate", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );

	/*if open failed*/
	if(fd2<0){
		perror("ERROR: ");
		close(fd1);
		free(new_val);
		exit(1);
	}

	val = (char*)malloc(10*sizeof(char));

	/*reading parameter value*/
	size = read(fd2,val,10); //current value of blink_rate

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd1);
		close(fd2);
		free(new_val);
		free(val);
		exit(1);
	}

	val[size-1]='\0';

	/*print read value*/
	printf("Current value of blink_rate: %s\n",val);

	/*close parameter file*/
	if(close(fd2)<0){
		perror("ERROR: ");
		close(fd1);
		free(new_val);
		free(val);
		exit(1);
	}

	/*open parameter file again*/
	fd2 = open("/sys/module/hw4/parameters/blink_rate", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open fails*/
	if((fd2)<0){
		perror("ERROR: ");
		close(fd1);
		free(new_val);
		free(val);
		exit(1);
	}

	/*modify parameter blink_rate*/
	size = write(fd2,new_val,10);
	
	/*if writing fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd1);
		close(fd2);
		free(val);
		free(new_val);
		exit(1);
	}

	if(close(fd2)<0){
		perror("ERROR: ");
		free(val);
		close(fd1);
		free(new_val);
		exit(1);
	}


	/*open parameter file again*/
	fd2 = open("/sys/module/hw4/parameters/blink_rate", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open fails*/
	if((fd2)<0){
		perror("ERROR: ");
		close(fd1);
		free(new_val);
		free(val);
		exit(1);
	}


	/*reading new value of parameter blink_rate*/
	size = read(fd2,val,10);
	val[size-1]='\0';

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd1);
		close(fd2);
		free(val);
		free(new_val);
		exit(1);
	}
	
	/*print new value*/
	printf("New value of blink_rate: %s\n",val);

	if(close(fd2)<0){
		perror("ERROR: ");
		free(val);
		close(fd1);
		free(new_val);
		exit(1);
	}

	if(close(fd1)<0){
		perror("ERROR: ");
		free(val);
		free(new_val);
		exit(1);
	}

	free(val);
	printf("file(s) closed\n");
	return 0;
}
