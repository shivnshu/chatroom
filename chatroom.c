#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/list.h>
#include<linux/slab.h>

#include "chatroom.h"

#define MAX_ONLINE_PROCESSES 16
#define DEVNAME "chatroom"

char online_process_handles[MAX_ONLINE_PROCESSES*(33)];

struct chatroom_process {
        char handle[32];
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_process init_process;
struct chatroom_process *tmp_process;

struct chatroom_message {
        char message[128];
        char handle[32];
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_message init_message;
struct chatroom_message *tmp_message;

struct list_head *pos, *next;


static int chatroom_open(struct inode *inode, struct file *file)
{
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Chatroom opened successfully\n");
        return 0;
}

static int chatroom_release(struct inode *inode, struct file *file)
{
        module_put(THIS_MODULE);
        printk(KERN_INFO "Chatroom closed successfully\n");
        return 0;
}

static ssize_t chatroom_read(struct file *filp,
                              char *buffer,
                              size_t length,
                              loff_t *offset)
{
        printk(KERN_INFO "Not supported read\n");
        return -EINVAL;
}

static ssize_t chatroom_write(struct file *filp,
                                const char *buff,
                                size_t length,
                                loff_t *offset)
{
        printk(KERN_INFO "Not supported write\n");
        return -EINVAL;
}

static long chatroom_ioctl(struct file *file,
                            unsigned int ioctl_num,
                            unsigned long arg)
{
        int i = 0;
        int retval = -EINVAL;
        switch(ioctl_num){
                case IOCTL_LOGIN:
                        tmp_process = (struct chatroom_process *)kmalloc(sizeof(struct chatroom_process), GFP_KERNEL);
                        strncpy(tmp_process->handle, (char *)arg, 32);
                        tmp_process->timestamp = jiffies;
                        list_add(&(tmp_process->list), &(init_process.list));
                        retval = 0;
                        printk(KERN_INFO "Login by handle %s\n", (char *)arg);
                        printk(KERN_INFO "handle: %s, timestamp: %lu", tmp_process->handle, tmp_process->timestamp);
                        break;
                case IOCTL_LOGOUT:
                        list_for_each_safe(pos, next, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (!strcmp(tmp_process->handle, (char *)arg)) {
                                        list_del(pos);
                                        kfree(tmp_process);
                                }
                        }
                        printk(KERN_INFO "Logout by handle %s\n", (char *)arg);
                        retval = 0;
                        break;
                case IOCTL_CHECKLOGIN:
                        /*list_for_each(pos, &init_process.list) {*/
                                /*tmp_process = list_entry(pos, struct chatroom_process, list);*/
                                /*strcpy((online_process_handles+i), tmp_process->handle);*/
                                /*i += strlen(tmp_process->handle);*/
                        /*}*/
                        /*while (i < sizeof(online_process_handles))*/
                                /*online_process_handles[i++] = '\0';*/
                        /**((char **)(arg)) = &online_process_handles[0];*/
                        /*retval = 0;*/
                        break;
                default:
                        printk(KERN_INFO "Sorry, this ioctl operation is not supported\n");
        }
        return retval;
}

static struct file_operations fops = {
        .read = chatroom_read,
        .write = chatroom_write,
        .open = chatroom_open,
        .release = chatroom_release,
        .unlocked_ioctl = chatroom_ioctl,
};

int init_module(void)
{
        int ret;
        printk(KERN_INFO "Chatroom: Hello kernel\n");
        ret = register_chrdev(DEV_MAJOR, DEVNAME, &fops);
        if (ret < 0) {
                printk(KERN_ALERT "Registering chatroom failed with %d\n", ret);
                return ret;
        }

        printk(KERN_INFO "I was assigned major number %d. To talk to\n", DEV_MAJOR);
        printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVNAME, DEV_MAJOR);

        INIT_LIST_HEAD(&init_process.list);
        INIT_LIST_HEAD(&init_message.list);

        return 0;
}

void cleanup_module(void)
{
        unregister_chrdev(DEV_MAJOR, DEVNAME);
        printk(KERN_INFO "Chatroom: GoodBye kernel\n");
}

MODULE_AUTHOR("shivansh@iitk.ac.in");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ChatRoom");
