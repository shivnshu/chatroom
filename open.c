#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "chatroom.h"

int main(int argc, char **argv)
{
        if (argc < 3) {
                printf("Usage: ./bin <handle>  (login|logout)");
                return 0;
        }
        int i;
        char handle[16];
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        strncpy(handle, argv[1], 16);

        if (!strcmp(argv[2], "login"))
                ioctl(fd, IOCTL_LOGIN, handle);

        else
                ioctl(fd, IOCTL_LOGOUT, handle);

        close(fd);
        return 0;
}

