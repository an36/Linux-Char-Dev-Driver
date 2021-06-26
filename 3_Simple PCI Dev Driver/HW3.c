/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 5/8/2020
 * ECE 373
 *
 * HW3: kernel module with .probe() and .remove() for pci device driver
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

#define DEVCNT 5
#define DEVNAME "HW3"

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

/* this shows up under /sys/modules/HW3/parameters */
static int exam;
module_param(exam, int, S_IRUSR | S_IWUSR);


static int HW3_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "successfully opened!\n");

	mydev.syscall_val = exam;

	return 0;
}

static ssize_t HW3_read(struct file *file, char __user *buf,
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
	
	/*Assign the value of our physical address to syscall_val*/
	mydev.syscall_val = ioread32(mydev.hw_addr+0x00E00);

	if (copy_to_user(buf, &mydev.syscall_val, sizeof(int))) {
		ret = -EFAULT;
		goto out;
	}

	ret = sizeof(int);
	*offset += sizeof(int);

	/* Good to go, so printk the thingy */
	printk(KERN_INFO "LED register: syscall_val= 0x%06X\n", mydev.syscall_val);

out:
	return ret;
}

static ssize_t HW3_write(struct file *file, const char __user *buf,
                              size_t len, loff_t *offset)
{
	/* Have local kernel memory ready */
	u_int32_t *kern_buf;
	int ret;

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

	/*changing the kern_buf to 32-bit number to hold the LED register address*/
	iowrite32(*((u32*)kern_buf),mydev.hw_addr+0x00E00);
	
	/*Assigning value to syscall_val*/
	//mydev.syscall_val = *kern_buf;

	/* print what userspace gave us */
	printk(KERN_INFO "Userspace wrote %X to syscall_val\n",(unsigned)*kern_buf);
	
	
mem_out:
	kfree(kern_buf);
out:
	return ret;
}

/* File operations for our device */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = HW3_open,
	.read = HW3_read,
	.write = HW3_write,
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
	pci_request_selected_regions(pdev,mydev.BAR,"HW3_driver");	//setting the driver name inside the pci table
	
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
	.name= "HW3_driver",
	.id_table= my_table,
	.probe= my_probe,
	.remove= my_remove,
};



static int __init HW3_init(void)
{
	
	printk("HW3 module loading... exam=%d\n",exam);

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
	return pci_register_driver(&mypci_driver);  //registering PCI device driver


//	return 0;
}

static void __exit HW3_exit(void)
{
	/* destroy the cdev */
	cdev_del(&mydev.my_cdev);

	/* clean up the devices */
	unregister_chrdev_region(mydev.mydev_node, DEVCNT);
	
	pci_unregister_driver(&mypci_driver);	//unregistering PCI device driver

	printk(KERN_INFO "HW3 module unloaded!\n");
	printk("released\n");

}

MODULE_AUTHOR("HW3");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(HW3_init);
module_exit(HW3_exit);
