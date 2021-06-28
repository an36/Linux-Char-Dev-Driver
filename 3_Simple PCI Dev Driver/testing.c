/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 5/8/20
 *
 * testing: opens, reads and writes to "/dev/ece" to turn on and off
 * 			an LED.
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
     u_int32_t on = 0xE;	//Sets LED on in MODE register
	u_int32_t off = 0xF;	//Sets LED off in MODE register
	u_int32_t mask = 0xFFFFFFF0; //value to mask the LED register value
	u_int32_t buf;

	/*opening the kernel module node file /dev/ece*/
	int fd = open("/dev/ece", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );

	/*if open failed*/
	if(fd<0){
		perror("ERROR: ");
		exit(1);
	}

	/*reading LED register value*/
	size = read(fd,&buf,sizeof(u_int32_t));

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}

	/*print read value*/
	printf("Current LED register value: %X\n",buf);


	/*close the file*/
	if(close(fd)<0){
		perror("ERROR: ");
		exit(1);
	}

	/*Assigning value to turn on LED*/
	buf = (buf & mask) | on;


	/*open file again*/
	fd = open("/dev/ece", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open fails*/
	if((fd)<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}

	/*modify LED register value*/
	size = write(fd,&buf,sizeof(u_int32_t));
	
	/*if writing fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}

	
	lseek(fd,0,SEEK_SET);

	/*reading new value of LED register*/
	size = read(fd,&buf,sizeof(u_int32_t));

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}
	
	/*print new value*/
	printf("new value of LED register (LED on): %X\n",buf);

	if(close(fd)<0){
		perror("ERROR: ");
		exit(1);
	}


	sleep(2); //sleep for 2 seconds

	/*Assigning value to turn off LED*/
	buf = (buf & mask) | off;

	/*open file again*/
	fd = open("/dev/ece", O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO );
	
	/*if open fails*/
	if((fd)<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}

	/*modify LED register value*/
	size = write(fd,&buf,sizeof(u_int32_t));
	
	/*if writing fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}

	
	lseek(fd,0,SEEK_SET);

	/*reading new value of LED register*/
	size = read(fd,&buf,sizeof(u_int32_t));

	/*if reading fails*/
	if(size<0){
		perror("ERROR: ");
		close(fd);
		exit(1);
	}
	
	/*print new value*/
	printf("new value of LED register (LED off): %X\n",buf);

	if(close(fd)<0){
		perror("ERROR: ");
		exit(1);
	}



	printf("file closed\n");
	return 0;
}
