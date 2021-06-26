/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 4/22/20
 * ECE373
 *
 * testing: opens, reads and writes to "/sys/module/HW2_2/parameters/exam" 
 * 		module parameter to view and modify the value of 
 * 		"exam" in HW2_2 module.  The value of syscall_val will be
 * 		changed too because syscall_val=exam.
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(){
	
	int size;
        char *val;
	
	/*opening the kernel module parameter (called exam)*/
	int fd = open("/sys/module/HW2_2/parameters/exam", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );

	/*if open failed*/
	if(fd<0){
		perror("ERROR: ");
		exit(1);
	}

	val = (char*)malloc(10*sizeof(char));

	/*reading parameter value*/
	size = read(fd,val,10);
	val[size-1]='\0';

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		free(val);
		close(fd);
		exit(1);
	}
	
	/*print read value*/
	printf("parameter exam: %s\n",val);


	/*close the parameter file*/
	if(close(fd)<0){
		perror("ERROR: ");
		free(val);
		exit(1);
	}

	/*open parameter file again*/
	fd = open("/sys/module/HW2_2/parameters/exam", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open fails*/
	if((fd)<0){
		perror("ERROR: ");
		free(val);
		close(fd);
		exit(1);
	}

	/*modify parameter exam*/
	size = write(fd,"36",2);
	
	/*if writing fails*/
	if(size<0){
		perror("ERROR: ");
		free(val);
		close(fd);
		exit(1);
	}


	char *newval = (char*)malloc(10*sizeof(char));
	
	lseek(fd,0,SEEK_SET);

	/*reading new value of parameter exam*/
	size = read(fd,newval,10);
	newval[size-1]='\0';

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		free(val);
		free(newval);
		close(fd);
		exit(1);
	}
	
	/*print new value*/
	printf("new value of parameter exam: %s\n",newval);

	if(close(fd)<0){
		perror("ERROR: ");
		free(val);
		free(newval);
		exit(1);
	}


	printf("file closed\n");
	free(val);
	free(newval);

	return 0;
}
