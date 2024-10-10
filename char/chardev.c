/* 
 * 简单字符串驱动设计思路
 * 1.字符设备的注册和初始化
 * 2.实现文件操作函数
 * 3.处理设备的打开、读写、关闭等操作
 * 4.内核和用户空间的数据交互
 * 5.模块的加载和卸载
 */

/*
 * 1.字符设备的注册和初始化
 * 每个字符设备在内核中都需要有一个主设备号（major number）来标识设备类型，以及从一个从设备号（minor number）来标识具体的设备实例。
 * **注册字符设备** 使用register_chrdev()函数向内核注册字符设备驱动程序，为该设备分配一个主设备号；
 * **创建设备节点** 设备文件节点通过 mknod 或 device_create() 创建，用户通过 /dev/simple_chardev 访问设备；
 *                通过 class_create() 创建设备类，通过 device_create() 创建具体的设备节点（如/dev/simple_chardev）；
 *                这两步能够在用户空间通过/dev/下的设备节点与字符设备驱动交互。
 * 
 * register_chrdev()  向内核注册字符设备驱动程序
 * class_create()     创建设备类，生成 /sys/class/ 目录下的一个子目录，代表该类的设备
 * device_create()    创建设备节点，允许用户空间程序访问该设备
 */

/*
 * 2.实现文件操作函数指针表
 * 字符设备通过一系列文件操作与用户空间
 * open() 当用户打开设备文件时调用，通常用于初始化设备或为读写做准备。
 * read() 当用户从设备文件中读取数据时调用，通常用于从设备读取数据。
 * write() 当用户向设备文件中写入数据时调用，通常用于向设备写入数据。
 * release() 当用户关闭设备文件时调用，通常用于释放资源。
 * 这些函数需要通过 file_operations 结构体来定义，并将其与字符设备驱动程序关联起来。
 */

/*
 * 3.处理设备的打开、读写、关闭操作
 * **打开设备**：每次用户使用open()打开设备文件时，内核会调用设备驱动程序中的open()函数；
 * **读取设备**：当用于使用cat等命令从设备中读取数据时，内核会调用驱动的read()函数；
 *             read()函数的任务是从设备中读取数据，使用copy_to_user()函数将数据从内核空间复制到用户空间；
 * **写入设备**：当用户使用echo等命令向设备中写入数据时，内核会调用驱动的write()函数；
 *             write()函数使用copy_from_user()函数将数据从用户空间复制到内核空间； 
 * **关闭设备**：当用户使用close()关闭设备文件时，内核会调用驱动的release()函数，用于清理资源或重置设备状态
 */

/*
 * 4.内核空间与用户空间的数据交互
 * 内核空间和用户空间之间的数据交互需要使用 copy_to_user() 和 copy_from_user() 函数；
 * copy_to_user() 从内核空间复制数据到用户空间；
 * copy_from_user() 从用户空间复制数据到内核空间；
 */

/*
 * 5.模块的加载和卸载
 * **加载模块**：模块加载时会执行 chardev_init 函数。注册字符设备、创建设备类、创建设备节点；
 * **卸载模块**：模块卸载时会执行 chardev_exit 函数。销毁设备节点、注销类、注销字符设备驱动程序；
 */



#include <linux/module.h>      // 模块头文件
#include <linux/fs.h>          // 文件操作头文件
#include <linux/cdev.h>        // 字符设备头文件
#include <linux/device.h>      // 设备模型头文件
#include <linux/uaccess.h>     // 用户空间访问内核空间头文件    

#define DEVICE_NAME "simple_chardev"  // 设备名
#define CLASS_NAME  "simple_class"    // 类名
#define BUFFER_SIZE 1024              // 缓冲区大小

static int    majorNumber;                // 主设备号
static char   message[BUFFER_SIZE] = {0}; // 缓冲区    
static short  message_size;               // 缓冲区大小
static struct class  *charClass  = NULL;    // 类
static struct device *charDevice = NULL;  // 设备结构体

// 文件操作函数
static int     dev_open(struct inode *, struct file *); // 打开设备
static int     dev_release(struct inode *, struct file *); // 释放设备
static ssize_t dev_read(struct file *, char *, size_t, loff_t *); // 读设备
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *); // 写设备

// 文件操作结构体
static struct file_operations fops = 
{
    .open    = dev_open,
    .read    = dev_read,
    .write   = dev_write,
    .release = dev_release,
};

// 初始化模块时调用的函数
static int __init charDev_init(void) { 
    printk(KERN_INFO "CharDevice: Initializing the CharDevice\n");
    
    // 注册字符设备驱动程序
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "CharDevice failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "CharDevice: registered with major number %d\n", majorNumber);
    
    // 注册类
    charClass = class_create(CLASS_NAME);
    if (IS_ERR(charClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "CharDevice failed to register a class\n");
        return PTR_ERR(charClass);
    }
    printk(KERN_INFO "CharDevice: device class registered successfully\n");
     
    // 创建设备节点
    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice)) { 
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(charDevice);
    }
    printk(KERN_INFO "CharDevice: device class created successfully\n");
    return 0;
}

// 卸载模块时调用的函数
static void __exit charDev_exit(void) {
    device_destroy(charClass, MKDEV(majorNumber, 0));      // 移除设备
    class_unregister(charClass);                           // 注销类
    class_destroy(charClass);                              // 销毁类
    unregister_chrdev(majorNumber, DEVICE_NAME);           // 注销字符设备驱动程序
    printk(KERN_INFO "CharDevice: Goodbye from the kernel!\n");
}

// 打开设备文件
static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "CharDevice: Device has been opened\n");
    return 0;
}

// 从设备读取数据
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;

    // 如果已经读取所有数据，返回0表示EOF
    if (*offset >= message_size) {
        return 0;
    }

    // 计算剩余未读的数据量
    if (len > message_size - *offset) {
        len = message_size - *offset;
    }

    // 将内核空间的数据复制到用户空间
    error_count = copy_to_user(buffer, message, message_size);

    if (error_count == 0) { 
        printk(KERN_INFO "CharDevice: Sent %zu characters to the user\n", len);
        *offset += len; // 更新偏移量
        return (len);   // 返回实际读取的字节数
    } else {
        printk(KERN_INFO "CharDevice: Failed to send %d characters to the user\n", error_count);
        return -EFAULT; // 发生错误时返回 -EFAULT
    }
}

// 向设备写入数据
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    if (len > BUFFER_SIZE) {
        len = BUFFER_SIZE - 1; // 防止溢出
    }
    
    // 从用户空间复制数据到内核空间
    if (copy_from_user(message, buffer, len)) {
        return -EFAULT;
    }

    message[len] = '\0';
    message_size = len;

    printk(KERN_INFO "CharDevice: Received %zu characters from the user\n", len);
    return len;
}

// 释放设备
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "CharDevice: Device successfully closed\n");
    return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ku Jui");
MODULE_DESCRIPTION("A simple Linux char device driver");
MODULE_VERSION("1.0");

module_init(charDev_init); // 指定模块初始化函数
module_exit(charDev_exit); // 指定模块卸载函数