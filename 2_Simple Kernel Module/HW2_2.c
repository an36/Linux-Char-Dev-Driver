/*
 * Abdullah Almarzouq	(an36@pdx.edu)
 * 4/22/2020
 *
 * HW2_2: kernel module with 1 parameter called exam
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVCNT 5
#define DEVNAME "HW2_2"

static struct mydev_dev {
	struct cdev my_cdev;
	dev_t mydev_node;
	int syscall_val;
} mydev;


/* this shows up under /sys/modules/HW2_2/parameters */
static int exam;
module_param(exam, int, S_IRUSR | S_IWUSR);


static int HW2_2_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "successfully opened!\n");

	mydev.syscall_val = exam;

	return 0;
}

static ssize_t HW2_2_read(struct file *file, char __user *buf,
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

static ssize_t HW2_2_write(struct file *file, const char __user *buf,
                              size_t len, loff_t *offset)
{
	/* Have local kernel memory ready */
	char *kern_buf;
	int ret, res;

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
	
	/* print what userspace gave us */
	printk(KERN_INFO "Userspace wrote \"%s\" to syscall_val\n", kern_buf);
	
	/*converting ascii/string to int*/
	res= kstrtoint(kern_buf,10,&mydev.syscall_val);
	if(res){
		ret= -EINVAL;
		goto out;
	}

mem_out:
	kfree(kern_buf);
out:
	return ret;
}

/* File operations for our device */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = HW2_2_open,
	.read = HW2_2_read,
	.write = HW2_2_write,
};

static int __init HW2_2_init(void)
{
	
	printk("HW2_2 module loading... exam=%d\n",exam);

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

	return 0;
}

static void __exit HW2_2_exit(void)
{
	/* destroy the cdev */
	cdev_del(&mydev.my_cdev);

	/* clean up the devices */
	unregister_chrdev_region(mydev.mydev_node, DEVCNT);

	printk(KERN_INFO "HW2_2 module unloaded!\n");
}

MODULE_AUTHOR("HW2_2");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(HW2_2_init);
module_exit(HW2_2_exit);
