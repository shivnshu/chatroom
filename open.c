#include<stdio.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>

#include "chatroom.h"

int main()
{
        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }
        
        char handle[16] = {'a', 's', 'd', 'f'};

        if (ioctl(fd, IOCTL_LOGIN, handle) < 0) {
                perror("ioctl");
        }

        if (ioctl(fd, IOCTL_LOGOUT, handle) < 0) {
                perror("ioctl");
        }

 
        close(fd);

        return 0;
}

