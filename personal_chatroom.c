#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/list.h>
#include<linux/slab.h>
#include<linux/uaccess.h>
#include<linux/mutex.h>

#include "chatroom.h"

#define DEVNAME "chatroom"

struct chatroom_process {
        char handle[HANDLE_SIZE];
        pid_t pid;
        unsigned long last_msg_read_timestamp;
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_process init_process;
struct chatroom_process *tmp_process;

struct chatroom_message {
        char message[MESSAGE_SIZE];
        char handle[HANDLE_SIZE];
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_message init_message;
struct chatroom_message *tmp_message;

struct list_head *pos, *next;

struct mutex process_buf_mutex;
struct mutex message_buf_mutex;


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
        int flag = 0;
        mutex_lock_interruptible(&process_buf_mutex);
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        break;
                }
        }
        mutex_unlock(&process_buf_mutex);

        if (!flag) {
                printk(KERN_INFO "Process is not logged in\n");
                return -EINVAL;
        }
        flag = 0;

        mutex_lock_interruptible(&message_buf_mutex);
        list_for_each(pos, &init_message.list) {
                tmp_message = list_entry(pos, struct chatroom_message, list);
                if (tmp_message->timestamp > tmp_process->last_msg_read_timestamp) {
                        tmp_process->last_msg_read_timestamp = tmp_message->timestamp;
                        flag = 1;
                        break;
                }
        }
        mutex_unlock(&message_buf_mutex);

        if (!flag) {
                printk(KERN_INFO "No message left to read by %s\n", tmp_process->handle);
                return -EINVAL;
        }
        if (length > MESSAGE_SIZE)
                length = MESSAGE_SIZE;
        copy_to_user(buffer, tmp_message->message, length);

        flag = 0;

        mutex_lock_interruptible(&process_buf_mutex);
        pos = tmp_process->list.next;
        mutex_unlock(&process_buf_mutex);
        if (pos != &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (tmp_process->timestamp >= tmp_message->timestamp)
                        flag = 1;
        }

        if (flag || pos == &init_process.list) {
                mutex_lock_interruptible(&message_buf_mutex);
                list_del(&tmp_message->list);
                kfree(tmp_message);
                mutex_unlock(&message_buf_mutex);
        }

        return length;

}

static ssize_t chatroom_write(struct file *filp,
                                const char *buff,
                                size_t length,
                                loff_t *offset)
{
        int flag = 0;
        mutex_lock_interruptible(&process_buf_mutex);
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        break;
                }
        }
        mutex_unlock(&process_buf_mutex);
        if (!flag) {
                printk(KERN_INFO "Process is not logged in\n");
                return -EINVAL;
        }
        tmp_message = (struct chatroom_message *)kmalloc(sizeof(struct chatroom_message), GFP_KERNEL);
        memset(tmp_message->message, 0, MESSAGE_SIZE);
        if (length > MESSAGE_SIZE)
                length = MESSAGE_SIZE;
        copy_from_user(tmp_message->message, buff, length);
        strncpy(tmp_message->handle, tmp_process->handle, HANDLE_SIZE);
        tmp_message->timestamp = jiffies;
        mutex_lock_interruptible(&message_buf_mutex);
        list_add_tail(&(tmp_message->list), &(init_message.list)); // Implement Queue
        mutex_unlock(&message_buf_mutex);
        return length;
}

static long chatroom_ioctl(struct file *file,
                            unsigned int ioctl_num,
                            unsigned long arg)
{
        int i = 0;
        int flag = 0;
        char **tmp;
        int retval = -EINVAL;
        switch(ioctl_num){
                case IOCTL_LOGIN:
                        mutex_lock_interruptible(&process_buf_mutex);
                        list_for_each(pos, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (!strcmp(tmp_process->handle, (char*)arg)) {
                                        printk(KERN_INFO "Handle with this name already exists\n");
                                        flag = 1;
                                        break;
                                }
                        }
                        mutex_unlock(&process_buf_mutex);
                        if (flag) {
                                retval = 0;
                                break;
                        }
                        tmp_process = (struct chatroom_process *)kmalloc(sizeof(struct chatroom_process), GFP_KERNEL);
                        strncpy_from_user(tmp_process->handle, (char *)arg, HANDLE_SIZE);
                        tmp_process->timestamp = jiffies;
                        tmp_process->last_msg_read_timestamp = jiffies;
                        tmp_process->pid = current->pid;
                        mutex_lock_interruptible(&process_buf_mutex);
                        list_add_tail(&(tmp_process->list), &(init_process.list));
                        mutex_unlock(&process_buf_mutex);
                        retval = 0;
                        printk(KERN_INFO "Login by handle %s\n", (char *)arg);
                        /*printk(KERN_INFO "handle %s, timestamp %lu pid %d", tmp_process->handle, tmp_process->timestamp, tmp_process->pid);*/
                        break;
                case IOCTL_LOGOUT:
                        mutex_lock_interruptible(&process_buf_mutex);
                        list_for_each_safe(pos, next, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (!strcmp(tmp_process->handle, (char *)arg)) {
                                        flag = 1;
                                        list_del(pos);
                                        kfree(tmp_process);
                                        break;
                                }
                        }
                        if (!flag) {
                                printk(KERN_INFO "Process is not logged in\n");
                                retval = 0;
                                break;
                        }
                        if (list_empty(&init_process.list)) {
                                list_for_each_safe(pos, next, &init_message.list) {
                                        tmp_message = list_entry(pos, struct chatroom_message, list);
                                        list_del(pos);
                                        kfree(tmp_message);
                                }
                        }
                        mutex_unlock(&process_buf_mutex);
                        printk(KERN_INFO "Logout by handle %s\n", (char *)arg);
                        retval = 0;
                        break;
                case IOCTL_CHECKLOGIN:
                        tmp = (char **)arg;
                        mutex_lock_interruptible(&process_buf_mutex);
                        list_for_each(pos, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                copy_to_user((char *)(tmp+i), tmp_process->handle, 32); 
                                i += HANDLE_SIZE/sizeof(long);
                                /*printk(KERN_INFO "process %s\n", tmp_process->handle);*/
                        }
                        mutex_unlock(&process_buf_mutex);
                        retval = 0;
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
        mutex_init(&process_buf_mutex);
        mutex_init(&message_buf_mutex);

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
