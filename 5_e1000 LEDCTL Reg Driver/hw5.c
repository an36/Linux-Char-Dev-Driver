/*
 *	Author: Abdullah Almarzouq (an36@pdx.edu)
 *	5/22/20
 *	ECE373 | Homework 5
 *
 *	hw5.c: This program works as a userspace driver that reads
 *		and modifies the LEDCTL register to turn the LEDs on
 *		and off. It accesses the LEDCTL register by memory
 *		mapping the e1000 driver.
 *
 *	Note: All of the functions were supplied by ledmon.c
 *
 *	Arguments:
 *		-s	<bus:slot.function>
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pci/pci.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <linux/types.h>
#include <errno.h>

#define IFNAMSIZ 16

#define MEM_WINDOW_SZ  0x00010000

/* LED Control */
#define E1000_LEDCTL	0x00E00	/* LED Control - RW */

#define E1000_GPRR	0x04074

volatile void *e1000e_mem;
char *portname;
char *pci_bus_slot;

/* map the network device into our memory space */
int dev_mmap(off_t base_addr, volatile void **mem)
{
	int fd;

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	*mem = mmap(NULL, MEM_WINDOW_SZ, (PROT_READ|PROT_WRITE), MAP_SHARED, fd,
			base_addr);
	if (*mem == MAP_FAILED) {
		perror("mmap/readable - try rebooting with iomem=relaxed");
		close(fd);
		return -1;
	}

	return fd;
}

/* write to a device register */
void m_write(u32 reg, u32 value)
{
	u32 *p = (u32 *)(e1000e_mem + reg);
	*p = value;
}

/* read from a device register */
u32 m_read(u32 reg)
{
	u32 *p = (u32 *)(e1000e_mem + reg);
	u32 v = *p;
	return v;
}

void usage(char *prog)
{
	fprintf(stderr, "Usage: %s -s <bus:slot.func>\n", prog);
}

int main(int argc, char **argv)
{
	int dev_mem_fd;			//hold the file descriptor retured by dev_mmap()
	char buf[128] = { 0 };		//used to check if the device exists
	char pci_entry[128] = { 0 };	//used to check if the device exists
	char addr_str[10] = { 0 };	//will hold the address which will be mmap-ed
	off_t base_addr;		//holds the base address for the device to be mmap-ed
	FILE *input;			//file descriptor
	int ch;				//used to check argv[]
	int len;			//used to check if the device exists
	u32 ledctl;			//holds the value of LEDCTL reg
	u32 ledctl_copy;		//holds a copy of LEDCTL
	u32 GPRR;			//hold the content of GPRR


	/*checking argv[]*/
	if (getuid() != 0) {
		fprintf(stderr, "%s: Must run as root.\n", argv[0]);
		exit(1);
	}

	if (argv[1] == NULL) {
		fprintf(stderr, "Usage: %s -s <bus:slot.func>\n", argv[0]);
		exit(1);
	}

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch (ch) {
		case 's':
			pci_bus_slot = optarg;
			break;
		default:
			fprintf(stderr, "unknown arg '%c'\n", ch);
			usage(argv[0]);
			exit(1);
			break;
		}
	}

	if (argv[optind] != NULL) {
		portname = argv[optind];
		

	/* Does it exist? */
		snprintf(buf, sizeof(buf), "ip -br link show %s", portname);
		input = popen(buf, "r");
		
		if (!input) {
			perror(portname);
			exit(1);
		}
		
		fgets(buf, sizeof(pci_entry), input);
		fclose(input);
		
		if (strncmp(portname, buf, strlen(portname))) {
			fprintf(stderr, "%s not found\n", portname);
			exit(1);
		}
	}
	
	if (!pci_bus_slot) {
		usage(argv[0]);
		exit(1);
	}
	
	/* Does pci device specified by the user exist? */
	snprintf(buf, sizeof(buf), "lspci -s %s", pci_bus_slot);
	input = popen(buf, "r");
	if (!input) {
		perror(pci_bus_slot);
		exit(1);
	}

	fgets(pci_entry, sizeof(pci_entry), input);
	fclose(input);
	len = strlen(pci_entry);
	if (len <= 1) {
		fprintf(stderr, "%s not found\n", pci_bus_slot);
		exit(1);
	}

	/* Let's make sure this is an Intel ethernet device.  A better
	 * way for doing this would be to look at the vendorId and the
	 * deviceId, but we're too lazy to find all the deviceIds that
	 * will work here, so we'll live a little dangerously and just
	 * be sure it is an Intel device according to the description.
	 * Oh, and this is exactly how programmers get into trouble.
	 */
	if (!strstr(pci_entry, "Ethernet controller") ||
	    !strstr(pci_entry, "Intel")) {
		fprintf(stderr, "%s wrong pci device\n", pci_entry);
		exit(1);
	}
	
	/* Only grab the first memory bar */
	snprintf(buf, sizeof(buf),
		 "lspci -s %s -v | awk '/Memory at/ { print $3 }' | head -1",
		 pci_bus_slot);
	input = popen(buf, "r");
	if (!input) {
		printf("%s\n", buf);
		perror("getting device mem info");
		exit(1);
	}
	fgets(addr_str, sizeof(addr_str), input);
	fclose(input);

    base_addr = strtol(addr_str, NULL, 16);
	if (len <= 1) {
        fprintf(stderr, "%s memory address invalid\n", addr_str);
        exit(1);
    }
	
	/*memory mapping*/
	dev_mem_fd = dev_mmap(base_addr, &e1000e_mem);
		
	ledctl = m_read(E1000_LEDCTL); //reading and saving LEDCTL value
	ledctl_copy = ledctl;

	printf("LEDCTL value: %u\n",ledctl);


	/*LED0 & LED2 on for 2 seconds*/
	printf("LED0 & LED2 on for 2 seconds\n");
	m_write(E1000_LEDCTL,0x0E0F0E);
	sleep(2);

	/*All LEDs off for 2 seconds*/
	printf("All LEDs off for 2 seconds\n");	
	m_write(E1000_LEDCTL,0x0F0F0F);
	sleep(2);

	for(int i=0;i<5;i++){
		printf("Looping around LED2, LED1 and LED0: LOOP[%d]\n",i+1);
		m_write(E1000_LEDCTL,0x0E0F0F);  //turn LED2 on
		sleep(1);
		m_write(E1000_LEDCTL,0x0F0E0F);  //turn LED1 on
		sleep(1);		
		m_write(E1000_LEDCTL,0x0F0F0E);  //turn LED0 on
		sleep(1);		
	}

	printf("Restoring LEDCTL register...\n");
	m_write(E1000_LEDCTL,ledctl_copy); //restore LEDCTL initial value
	
	GPRR = m_read(E1000_GPRR);	//Read contents of GPRR

	printf("Good Packets Recieved statics register: %u\n",GPRR);

	/*cleaning up*/
	close(dev_mem_fd);
	munmap((void *)e1000e_mem, MEM_WINDOW_SZ);
}
