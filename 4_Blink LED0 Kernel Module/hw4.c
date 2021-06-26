/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 5/22/2020
 * ECE 373
 *
 * hw4: Kernel module that controls the blinking of LED0 using a 
 * 		timer and blink_rate parameter. 
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/fcntl.h>
#include <linux/device.h>

#define DEVCNT 5
#define DEVNAME "HW4"
uint32_t on = 0x0F0F0F0E;
uint32_t off = 0x0F0F0F0F;
uint32_t msk = 0xFFFFFFF0;

static struct mydev_dev {
	struct cdev my_cdev;
	dev_t mydev_node;
	int syscall_val;
	struct pci_dev *pdev; //pointer to our pci device
	int BAR;	//base address register
	void* hw_addr;  // physical hardware address

} mydev;


static int my_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void my_remove(struct pci_dev *pdev);

static struct class *cls = NULL;  //to be used in class_create() and class_destroy() to create /dev/ece_led
static struct timer_list mytimer; //timer struct


/* this shows up under /sys/modules/hw4/parameters */
static int blink_rate = 2;  //default value is 2
module_param(blink_rate, int, S_IRUSR | S_IWUSR);

static int chk = 0; // checks if the led on or off in the timer_cb function

/*timer function*/
static void mytimer_cb(struct timer_list *t){


	/* check if blink_rate is invalid*/
	if(blink_rate<=0){
		blink_rate = 2; //reset blink_rate to default value
		printk("Error: Invalid Argument\n");
	}
	
	/*checks if led on then turn them off and vice versa*/
	if(chk==0){
		writel(off,(mydev.hw_addr+0x00E00));
		printk(KERN_INFO "LED turned off\n");
		chk=1;
	}
	else if(chk==1){
		writel(on,(mydev.hw_addr+0x00E00));
		printk(KERN_INFO "LED turned on\n");	
		chk=0;	
	}

	/*re-arm the timer*/
	mod_timer(&mytimer, (HZ/blink_rate)+jiffies);
}

static int HW4_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "successfully opened!\n");
	printk(KERN_INFO "LED0 is blinking...\n");


	mydev.syscall_val = blink_rate;

	mod_timer(&mytimer, (HZ/blink_rate)+jiffies);

	return 0;
}

static ssize_t HW4_read(struct file *file, char __user *buf,
                             size_t len, loff_t *offset)
{
	/* Get a local kernel buffer set aside */
	int ret;

	if (*offset >= sizeof(int)){
		return 0;
	}

	/* Make sure our user wasn't bad... */
	if (!buf) {
		ret = -EINVAL;
		goto out;
	}

	if (copy_to_user(buf, &mydev.syscall_val, sizeof(int))) {
		ret = -EFAULT;
		goto out;
	}

	ret = sizeof(int);
	*offset += sizeof(int);

	/* Good to go, so printk the thingy */
	printk(KERN_INFO "syscall_val= %d\n", mydev.syscall_val);

out:
	return ret;
}

static ssize_t HW4_write(struct file *file, const char __user *buf,
                              size_t len, loff_t *offset)
{
	/* Have local kernel memory ready */
	char *kern_buf;
	int ret,res;

	/* Make sure our user isn't bad... */
	if (!buf) {
		ret = -EINVAL;
		goto out;
	}

	/* Get some memory to copy into... */
	kern_buf = kmalloc(len, GFP_KERNEL);

	/* ...and make sure it's good to go */
	if (!kern_buf) {
		ret = -ENOMEM;
		goto out;
	}

	/* Copy from the user-provided buffer */
	if (copy_from_user(kern_buf, buf, len)) {
		/* uh-oh... */
		ret = -EFAULT;
		goto mem_out;
	}

	ret = len;
	
	/*converting ascii/string to int*/
	res = kstrtoint(kern_buf,10,&mydev.syscall_val);
	if(res){
		ret = -EINVAL;
		goto mem_out;
	}

	blink_rate = mydev.syscall_val;

	/* check if blink_rate is invalid*/
	if(blink_rate<0){
		blink_rate = 2; //reset blink_rate to default
		return -EINVAL;
	}

	/* print what userspace gave us */
	printk(KERN_INFO "Userspace wrote %d to syscall_val and blink_rate\n",mydev.syscall_val);
	
	
mem_out:
	kfree(kern_buf);
out:
	return ret;
}

/* File operations for our device */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = HW4_open,
	.read = HW4_read,
	.write = HW4_write,
};

/*.probe() function*/
static int my_probe(struct pci_dev *pdev, const struct pci_device_id *ent){

	resource_size_t mmio_start, mmio_len;
	
	/*enabling device memory access for our driver*/
	if(pci_enable_device_mem(pdev)){
		printk("Error: enabling memory access failed");
		return -1;
	}
	
	mydev.BAR = pci_select_bars(pdev,IORESOURCE_MEM);		//figuring out the bars
	pci_request_selected_regions(pdev,mydev.BAR,"HW4_driver");	//setting the driver name inside the pci table
	
	mmio_start = pci_resource_start(pdev,0);	//starting address of our bar
	mmio_len = pci_resource_len(pdev,0);		//figuring the length of our bar

	mydev.hw_addr = ioremap(mmio_start,mmio_len);	//returns the physical address

	return 0;
}


/*.remove() function*/
static void my_remove(struct pci_dev *pdev){
	
	struct __unmapping *unmap_dev= pci_get_drvdata(pdev);	//assigns the info of out device to unmap_dev
	iounmap(unmap_dev);	//unmaps the device
	pci_release_selected_regions(pdev, mydev.BAR);	//releases the device
}

/*setting up the device ID and vendor ID*/
static const struct pci_device_id my_table[]={
	{PCI_DEVICE(0x8086,0x100e)}
};

/*pci_driver structure holds driver info*/
static struct pci_driver mypci_driver = {
	.name= "HW4_driver",
	.id_table= my_table,
	.probe= my_probe,
	.remove= my_remove,
};



static int __init HW4_init(void)
{
	
	printk("HW4 module loading... blink_rate=%d\n",blink_rate);

	timer_setup(&mytimer,mytimer_cb,0); //setup the timer


	if (alloc_chrdev_region(&mydev.mydev_node, 0, DEVCNT, DEVNAME)) {
		printk(KERN_ERR "alloc_chrdev_region() failed!\n");
		return -1;
	}

	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT,
	       MAJOR(mydev.mydev_node));

	/* Initialize the character device and add it to the kernel */
	cdev_init(&mydev.my_cdev, &mydev_fops);
	mydev.my_cdev.owner = THIS_MODULE;

	if (cdev_add(&mydev.my_cdev, mydev.mydev_node, DEVCNT)) {
		printk(KERN_ERR "cdev_add() failed!\n");
		/* clean up chrdev allocation */
		unregister_chrdev_region(mydev.mydev_node, DEVCNT);

		return -1;
	}

	printk("hw_addr=%p\n",mydev.hw_addr);

	/*Creating a device node*/
	cls = class_create(THIS_MODULE, DEVNAME);
	if(cls==NULL){
		unregister_chrdev_region(mydev.mydev_node,DEVCNT);
		return -1;
	}

	if((device_create(cls,NULL,mydev.mydev_node,NULL,"ece_led"))==NULL){
		class_destroy(cls);
		unregister_chrdev_region(mydev.mydev_node,DEVCNT);
		printk("Error creating device file\n");
		return -1;
	}

	printk("Device node has been created: /dev/ece_led\n");

	

	return pci_register_driver(&mypci_driver);  //registering PCI device driver

}

static void __exit HW4_exit(void)
{
	/* destroy the cdev */
	cdev_del(&mydev.my_cdev);

	/* clean up the devices */
	unregister_chrdev_region(mydev.mydev_node, DEVCNT);
	
	pci_unregister_driver(&mypci_driver);	//unregistering PCI device driver

	device_destroy(cls,mydev.mydev_node);
	class_destroy(cls);
	del_timer_sync(&mytimer);

	printk(KERN_INFO "HW4 module unloaded!\n");
	printk("PCI Driver unregistered\n");
	printk("Device node (/dev/ece_led) destroyed\n");

}

MODULE_AUTHOR("Abdullah Almarzouq");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(HW4_init);
module_exit(HW4_exit);
