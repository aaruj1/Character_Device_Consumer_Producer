#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/sched.h>


#define  DEVICE_NAME "number_pipe"
#define  CLASS_NAME  "pipe"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antra Aruj");
MODULE_DESCRIPTION("character device driver for number pipe");


static struct class *charClass = NULL;
static struct device *charDevice = NULL;

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations chardevfops =
        {
                .open = device_open,
                .read = device_read,
                .write = device_write,
                .release = device_release,
        };

static char **buffer;
static int buffer_length = 100;

//read from the kernel commandline
module_param(buffer_length, int, 0000);

static int majorNumber;
static int deviceopen = 0;

static int readIndex = 0;
static int writeIndex = 0;

static int emptyCount = 0;
static int fullCount = 0;

static struct semaphore mutex;

static struct semaphore full;
static struct semaphore empty;

static int __init init_numpipe_module(void) {
    int i;
    printk(KERN_INFO "number_pipe: initializing the numpipe character device\n");

    majorNumber = register_chrdev(0, DEVICE_NAME, &chardevfops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "number_pipe: character device failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "number_pipe: device registered correctly with major number %d\n", majorNumber);

    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "number_pipe: failed to register device class\n");
        return PTR_ERR(charClass);
    }
    printk(KERN_INFO "number_pipe: device class registered correctly\n");

    // Register the device driver
    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice)) {
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "number_pipe: failed to create the device\n");
        return PTR_ERR(charDevice);
    }
    printk(KERN_INFO "number_pipe: device class created correctly\n");

    buffer = (char **) kmalloc(buffer_length * sizeof(char *), GFP_KERNEL);

    for (i = 0; i < buffer_length; i++) {
        buffer[i] = (char *) kmalloc(sizeof(long) + 1, GFP_KERNEL);
        buffer[i][sizeof(long)] = '\0';
    }

    if (!buffer){
        printk(KERN_ALERT "Error : Memory allocation failed\n");
    }


    emptyCount = buffer_length;
    sema_init(&full, 0);
    sema_init(&empty, buffer_length);
    sema_init(&mutex, 1);

    return 0;
}


static void __exit cleanup_numpipe_module(void) {
    int i;
    for (i = 0; i < buffer_length; i++) {
        kfree(buffer[i]);
    }
    device_destroy(charClass, MKDEV(majorNumber, 0));
    class_unregister(charClass);
    class_destroy(charClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "number_pipe: : goodbye from the module!\n");
}

static int device_open(struct inode *inodep, struct file *filep) {
    deviceopen++;
    printk(KERN_INFO "number_pipe: : device has been opened %d time(s)\n", deviceopen);
    return 0;
}

static ssize_t device_read(struct file *filep, char *outgoingBuffer, size_t len, loff_t *offset) {
    int error_count = 1;
    int readByte = 0;
    int downInterruptibleReturn;

    downInterruptibleReturn = down_interruptible(&full);
    if(downInterruptibleReturn < 0){
        printk(KERN_INFO "number_pipe: : Error in locking full semaphore\n");
        return -1;
    }

    downInterruptibleReturn = down_interruptible(&mutex);
    if(downInterruptibleReturn < 0){
        printk(KERN_INFO "number_pipe: : Error in locking read mutex\n");
        return -1;
    }

    if(readIndex >= buffer_length){
        readIndex = readIndex-buffer_length;
    }
    if (fullCount > 0) {
        error_count = copy_to_user(outgoingBuffer, buffer[readIndex], len);
        readIndex++;
        emptyCount++;
        fullCount--;
        readByte = len;
    }
    up(&mutex);
    up(&empty);

    if (error_count == 0) {
        printk(KERN_INFO "number_pipe: Successfully sent %ld bytes to the user\n", len);
        return (readByte);
    } else {
        printk(KERN_INFO "number_pipe: Failed to send %d bytes to the user\n", error_count);
        return (readByte);
    }
}

static ssize_t device_write(struct file *filep, const char *incomingBuffer, size_t len, loff_t *offset) {
    int error_count = 1;
    int writeByte = 0;
    int downInterruptibleReturn;

    downInterruptibleReturn = down_interruptible(&empty);
    if(downInterruptibleReturn <0) {
        printk(KERN_INFO "number_pipe: : Error in locking empty semaphore\n");
        return -1;
    }

    downInterruptibleReturn = down_interruptible(&mutex);

    if(downInterruptibleReturn <0){
        printk(KERN_INFO "number_pipe: : Error in locking write mutex\n");
        return -1;
    }

    if(writeIndex >= buffer_length){
        writeIndex = writeIndex - buffer_length;
    }
    if (emptyCount > 0) {
        error_count = copy_from_user(buffer[writeIndex], incomingBuffer, len);
        writeIndex++;
        fullCount++;
        emptyCount--;
        writeByte = len;
    }
    up(&mutex);
    up(&full);

    if(error_count ==0){
        printk(KERN_INFO "number_pipe: Successfully received %ld bytes to the user\n", len);
        return (writeByte);
    } else {
        printk(KERN_INFO "number_pipe: Failed to receive %d bytes to the user\n", error_count);
        return (writeByte);
    }
}

static int device_release(struct inode *inodep, struct file *filep) {
    deviceopen--;
    printk(KERN_INFO "number_pipe: Device successfully closed\n");
    return 0;
}

module_init(init_numpipe_module);
module_exit(cleanup_numpipe_module);
