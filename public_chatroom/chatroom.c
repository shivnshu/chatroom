#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/fs.h>
#include<linux/list.h>
#include<linux/slab.h>
#include<linux/uaccess.h>

#include "../chatroom.h"

#define DEVNAME "chatroom"

static long chatroom_ioctl(struct file *file,
                            unsigned int ioctl_num,
                            unsigned long arg);

// Details of each online process
struct chatroom_process {
        char handle[HANDLE_SIZE];
        pid_t pid; 
        unsigned long last_msg_read_timestamp; // timestamp of last message read
        unsigned long timestamp; // timestamp of login time
        struct list_head list; // doubly circular linked list
};
struct chatroom_process init_process;

// Details of each message
struct chatroom_message {
        char message[MESSAGE_SIZE];
        char handle[HANDLE_SIZE]; //  handle of process which sent message
        unsigned long timestamp; // timestamp of its origin
        struct list_head list; // doubly cicular linked list
};
struct chatroom_message init_message;

// R/W semaphores for protecting data of linked list of processes and messages
struct rw_semaphore process_sem;
struct rw_semaphore message_sem;

// Function called whenever any process open the devices
static int chatroom_open(struct inode *inode, struct file *file)
{
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Chatroom opened successfully\n");
        return 0;
}

// Function called whenever any process closes the devices either voluntarily or forced (if process crashes/dies)
static int chatroom_release(struct inode *inode, struct file *file)
{
        // if process dies, logout it
        chatroom_ioctl(file, IOCTL_LOGOUT, (unsigned long)NULL);
        module_put(THIS_MODULE);
        printk(KERN_INFO "Chatroom closed successfully\n");
        return 0;
}

static ssize_t chatroom_read(struct file *filp,
                              char *buffer,
                              size_t length,
                              loff_t *offset)
{
        struct chatroom_process *tmp_process;
        struct chatroom_message *tmp_message;
        struct list_head *pos;
        int flag = 0;
        // acquire write semaphore of processes list
        down_write(&process_sem);
        
        // iterate through each process of list, if pid matches, we get pointer to read caller process
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        break;
                }
        }

        // process is not in the list of online processes, then exit
        if (!flag) {
                printk(KERN_INFO "Process is not logged in\n");
                up_write(&process_sem);
                return -EINVAL;
        }
        flag = 0;

        // acquire messages list write semaphore
        down_write(&message_sem);
        list_for_each(pos, &init_message.list) {
                tmp_message = list_entry(pos, struct chatroom_message, list);
                // check if process has already read this message. If not, break
                if (tmp_message->timestamp > tmp_process->last_msg_read_timestamp) {
                        tmp_process->last_msg_read_timestamp = tmp_message->timestamp;
                        flag = 1;
                        break;
                }
        }

        // release processes write semaphore
        up_write(&process_sem);
        if (!flag) {
                // if no message remaining to read
                up_write(&message_sem);
                printk(KERN_INFO "No message left to read by %s\n", tmp_process->handle);
                return -EINVAL;
        }
        if (length > MESSAGE_SIZE)
                length = MESSAGE_SIZE;
        // copy the message to buffer
        copy_to_user(buffer, tmp_message->message, length);


        flag = 1;

        down_read(&process_sem);
        // for this message, check if there is any online process which hasn't read it
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (tmp_process->last_msg_read_timestamp < tmp_message->timestamp) {
                        flag = 0;
                        break;
                }
        }
        up_read(&process_sem);

        // if No, remove and free this message from linked list
        if (flag) {
                list_del(&tmp_message->list);
                kfree(tmp_message);
        }

        up_write(&message_sem);
        return length;

}

static ssize_t chatroom_write(struct file *filp,
                                const char *buff,
                                size_t length,
                                loff_t *offset)
{       
        struct chatroom_process *tmp_process;
        struct chatroom_message *tmp_message;
        struct list_head *pos;
 
        int flag = 0;
        down_read(&process_sem);
        // Get pointer of caller process by comparing pid
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        break;
                }
        }

        if (!flag) {
                up_read(&process_sem);
                printk(KERN_INFO "Process is not logged in\n");
                return -EINVAL;
        }
        tmp_message = (struct chatroom_message *)kmalloc(sizeof(struct chatroom_message), GFP_KERNEL);
        memset(tmp_message->message, 0, MESSAGE_SIZE);
        if (length > MESSAGE_SIZE)
                length = MESSAGE_SIZE;
        // fill tmp_message struct with appropriate data
        copy_from_user(tmp_message->message, buff, length);
        strncpy(tmp_message->handle, tmp_process->handle, HANDLE_SIZE);
        up_read(&process_sem);
        // Also set timestamp of message
        tmp_message->timestamp = jiffies;
        down_write(&message_sem);
        list_add_tail(&(tmp_message->list), &(init_message.list)); // Implements Queue
        up_write(&message_sem);
        return length;
}

static long chatroom_ioctl(struct file *file,
                            unsigned int ioctl_num,
                            unsigned long arg)
{
        struct chatroom_process *tmp_process;
        struct chatroom_message *tmp_message;
        struct list_head *pos, *next;
 
        int i = 0;
        int count = 0;
        int flag = 0;
        char **tmp;
        int retval = -EINVAL;
        switch(ioctl_num){
                case IOCTL_LOGIN:
                        down_read(&process_sem);
                        // Check if process with this handle is already online
                        list_for_each(pos, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (!strcmp(tmp_process->handle, (char*)arg)) {
                                        printk(KERN_INFO "Handle with this name already exists\n");
                                        flag = 1;
                                        break;
                                }
                        }
                        up_read(&process_sem);
                        if (flag) {
                                retval = 0;
                                break;
                        }
                        // Allocate new chatroom_process struct to be added in its linked list
                        tmp_process = (struct chatroom_process *)kmalloc(sizeof(struct chatroom_process), GFP_KERNEL);
                        strncpy_from_user(tmp_process->handle, (char *)arg, HANDLE_SIZE);
                        tmp_process->timestamp = jiffies;
                        tmp_process->pid = current->pid;
                        // last msg read timestamp is set to timestamp of this process
                        // (This process will read only those messages that have timestamp more than this value)
                        tmp_process->last_msg_read_timestamp = jiffies;
                        down_write(&message_sem);
                        list_add_tail(&(tmp_process->list), &(init_process.list));
                        up_write(&message_sem);
                        retval = 0;
                        printk(KERN_INFO "Login by handle %s\n", (char *)arg);
                        break;
                case IOCTL_LOGOUT:
                        down_write(&process_sem);
                        // Check if this is logged in
                        list_for_each_safe(pos, next, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (tmp_process->pid == current->pid) {
                                        flag = 1;
                                        list_del(pos);
                                        kfree(tmp_process);
                                        break;
                                }
                        }
                        up_write(&process_sem);
                        if (!flag) {
                                printk(KERN_INFO "Process is not logged in\n");
                                retval = 0;
                                break;
                        }
                        down_read(&process_sem);
                        // If there is no online process remaining, delete all messages
                        if (list_empty(&init_process.list)) {
                                down_write(&message_sem);
                                list_for_each_safe(pos, next, &init_message.list) {
                                        tmp_message = list_entry(pos, struct chatroom_message, list);
                                        list_del(pos);
                                        kfree(tmp_message);
                                }
                                up_write(&message_sem);
                        }
                        up_read(&process_sem);
                        printk(KERN_INFO "Logout by handle %s\n", (char *)arg);
                        retval = 0;
                        break;
                case IOCTL_CHECKLOGIN:
                        tmp = (char **)arg;
                        down_read(&process_sem);
                        // iterate through each online process and copy its handle to user supplied buffer
                        list_for_each(pos, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                copy_to_user((char *)(tmp+i), tmp_process->handle, 32); 
                                i += HANDLE_SIZE/sizeof(long);
                        }
                        up_read(&process_sem);
                        retval = 0;
                        break;
                case IOCTL_NUMMSG:
                        // Return no. of messages in the messages linked list
                        down_read(&message_sem);
                        list_for_each(pos, &init_message.list) {
                                count++;
                        }
                        up_read(&message_sem);
                        retval = count;
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

        // Initialise the linked lists
        INIT_LIST_HEAD(&init_process.list);
        INIT_LIST_HEAD(&init_message.list);
        
        // Initialise the r/w semaphores
        init_rwsem(&process_sem);
        init_rwsem(&message_sem);

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
