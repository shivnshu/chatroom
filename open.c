#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "chatroom.h"

char buf[MESSAGE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage: ./bin <handle>");
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

        ioctl(fd, IOCTL_LOGIN, handle);

        sleep(1);
        write(fd, "asdfghjk\0", MESSAGE_SIZE);
        sleep(1);
        read(fd, buf, MESSAGE_SIZE);
        printf("%s\n", buf);


        ioctl(fd, IOCTL_LOGOUT, handle);
        close(fd);
        return 0;
}
