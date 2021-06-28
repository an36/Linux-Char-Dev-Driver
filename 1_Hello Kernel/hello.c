/*
 * Author: Abdullah Almarzouq (an36@pdx.edu)
 *
 * The program below creates a "hello" module to be 
 * loaded into the kernel.
 *
 * April 12, 2020
 */

#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

static int __init hello_init(void){

	printk(KERN_INFO "Hello, kernel\n");
	return 0;
}

static void __exit hello_exit(void){

	printk(KERN_INFO "Goodbye, kernel\n");
}

module_init(hello_init);
module_exit(hello_exit);
