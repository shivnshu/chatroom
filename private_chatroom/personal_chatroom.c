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

struct chatroom_process {
        char handle[HANDLE_SIZE];
        pid_t pid;
        unsigned long last_msg_read_timestamp; // last msg read timestamp
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_process init_process;

struct chatroom_message {
        char message[MESSAGE_SIZE];
        char send_handle[HANDLE_SIZE]; // handle of process who send it
        // If recv_handle is empty => Public message to all online processes
        char recv_handle[HANDLE_SIZE]; // handle of process who will receive it
        unsigned long timestamp;
        struct list_head list;
};
struct chatroom_message init_message;

struct rw_semaphore process_sem;
struct rw_semaphore message_sem;

static int chatroom_open(struct inode *inode, struct file *file)
{
        try_module_get(THIS_MODULE);
        printk(KERN_INFO "Chatroom opened successfully\n");
        return 0;
}

static int chatroom_release(struct inode *inode, struct file *file)
{
        // Logout process in case of crash or process does not call LogOut before dying
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
        int recv_length;
        down_write(&process_sem);
        // Get ptr to caller process from online list of processes
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        break;
                }
        }

        if (!flag) {
                up_write(&process_sem);
                printk(KERN_INFO "Process is not logged in\n");
                return -EINVAL;
        }
        flag = 0;

        down_write(&message_sem);
        // Iterate through all elements of messages list, and try to match recv_handle to caller process
        // If strlen(recv_handle) == 0 => public message
        list_for_each(pos, &init_message.list) {
                tmp_message = list_entry(pos, struct chatroom_message, list);
                recv_length = strlen(tmp_message->recv_handle);
                if ((tmp_message->timestamp > tmp_process->last_msg_read_timestamp) && 
                                (!strcmp(tmp_message->recv_handle, tmp_process->handle) || !recv_length)) {
                        tmp_process->last_msg_read_timestamp = tmp_message->timestamp;
                        flag = 1;
                        break;
                }
        }
        up_write(&process_sem);

        if (!flag) {
                up_write(&message_sem);
                printk(KERN_INFO "No message left to read by %s\n", tmp_process->handle);
                return -EINVAL;
        }
        if (length > MESSAGE_SIZE)
                length = MESSAGE_SIZE;
        copy_to_user(buffer, tmp_message->message, length);

        flag = 1;

        // If message was public, check if there is no online process which hasn't read this message
        if (!recv_length) {
                down_read(&process_sem);
                list_for_each(pos, &init_process.list) {
                        tmp_process = list_entry(pos, struct chatroom_process, list);
                        if (tmp_process->last_msg_read_timestamp < tmp_message->timestamp) {
                                flag = 0;
                                break;
                        }
                }
                up_read(&process_sem);
                if (flag) {
                        list_del(&tmp_message->list);
                        kfree(tmp_message);
                }
        } else {
                // If message was private, delete it regardless
                list_del(&tmp_message->list);
                kfree(tmp_message);
        }
        up_write(&message_sem);

        return length;

}

// Initial HANDLE_SIZE bytes of the buff contains the recv_handle
// Message string starts from HANDLE_SIZE index of buff
static ssize_t chatroom_write(struct file *filp,
                                const char *buff,
                                size_t length,
                                loff_t *offset)
{
        struct list_head *pos;
        struct chatroom_process *tmp_process;
        struct chatroom_message *tmp_message;
        struct chatroom_process *current_process;
        int flag = 0;
        int flag_if_recv_process_exist = 0;
        int recv_length = strlen(buff);
        down_read(&process_sem);
        // Check if both caller process and recv_process is online, if not exit
        list_for_each(pos, &init_process.list) {
                tmp_process = list_entry(pos, struct chatroom_process, list);
                if (current->pid == tmp_process->pid) {
                        flag = 1;
                        current_process = tmp_process;
                }
                if (!strcmp(buff, tmp_process->handle)) {
                        flag_if_recv_process_exist = 1;
                }

        }
        up_read(&process_sem);
        if (!flag) {
                printk(KERN_INFO "Process is not logged in\n");
                return -EINVAL;
        }
        // If msg is public, no need to check for recv_process
        if (!flag_if_recv_process_exist && recv_length) {
                printk(KERN_INFO "%s process is not online\n", buff + HANDLE_SIZE);
                return -EINVAL;
        }
        tmp_message = (struct chatroom_message *)kmalloc(sizeof(struct chatroom_message), GFP_KERNEL);
        memset(tmp_message->message, 0, MESSAGE_SIZE);
        if (length > MESSAGE_SIZE + HANDLE_SIZE)
                length = MESSAGE_SIZE + HANDLE_SIZE;
        // Fill up the newly allocated message struct 
        copy_from_user(tmp_message->message, buff + HANDLE_SIZE, length - HANDLE_SIZE);
        strcpy(tmp_message->send_handle, current_process->handle);
        strcpy(tmp_message->recv_handle, buff);
        tmp_message->timestamp = jiffies;
        down_write(&message_sem);
        // Add this msg to linked list
        list_add_tail(&(tmp_message->list), &(init_message.list)); // Implement Queue
        up_write(&message_sem);
        return length;
}

static long chatroom_ioctl(struct file *file,
                            unsigned int ioctl_num,
                            unsigned long arg)
{
        struct list_head *pos, *next;
        struct chatroom_process *tmp_process;
        struct chatroom_message *tmp_message;
        char handle[HANDLE_SIZE];
        int i = 0;
        int flag = 0;
        int count = 0;
        char **tmp;
        int retval = -EINVAL;
        switch(ioctl_num){
                case IOCTL_LOGIN:
                        down_read(&process_sem);
                        // Ensure that no process with same handle is online
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
                        // Allocate new chatroom_process struct and fill up its data
                        tmp_process = (struct chatroom_process *)kmalloc(sizeof(struct chatroom_process), GFP_KERNEL);
                        strncpy_from_user(tmp_process->handle, (char *)arg, HANDLE_SIZE);
                        tmp_process->timestamp = jiffies;
                        tmp_process->pid = current->pid;
                        tmp_process->last_msg_read_timestamp = jiffies;
                        down_write(&process_sem);
                        list_add_tail(&(tmp_process->list), &(init_process.list));
                        up_write(&process_sem);
                        retval = 0;
                        printk(KERN_INFO "Login by handle %s\n", (char *)arg);
                        break;
                case IOCTL_LOGOUT:
                        down_write(&process_sem);
                        // Ensure that caller process is online
                        list_for_each_safe(pos, next, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                if (tmp_process->pid == current->pid) {
                                        flag = 1;
                                        strcpy(handle, tmp_process->handle);
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
                        // Delete all messages if no process is online
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
                        printk(KERN_INFO "Logout by handle %s\n", handle);
                        retval = 0;
                        break;
                case IOCTL_CHECKLOGIN:
                        // Fill user supplied buffer with all online processes handles
                        tmp = (char **)arg;
                        down_read(&process_sem);
                        list_for_each(pos, &init_process.list) {
                                tmp_process = list_entry(pos, struct chatroom_process, list);
                                copy_to_user((char *)(tmp+i), tmp_process->handle, 32); 
                                i += HANDLE_SIZE/sizeof(long);
                        }
                        up_read(&process_sem);
                        retval = 0;
                        break;
                case IOCTL_NUMMSG:
                        // Return no. of messages remaining (for debug purpose)
                        down_read(&message_sem);
                        list_for_each(pos, &init_message.list) {
                                count += 1;
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

        // Linked lists initialisation
        INIT_LIST_HEAD(&init_process.list);
        INIT_LIST_HEAD(&init_message.list);

        // r/w semaphores initialisation
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
