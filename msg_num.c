#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>

#include "chatroom.h"

int main()
{
        int i;
        int count;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        count = ioctl(fd, IOCTL_NUMMSG, NULL);
        if (count < 0) {
                perror("ioctl");
        }

        printf("%d\n", count);

        close(fd);

        return 0;
}
