#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "chatroom.h"

char buf[HANDLE_SIZE + MESSAGE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage: ./bin <handle>");
                return 0;
        }
        int i;
        char handle[HANDLE_SIZE];
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        strncpy(handle, argv[1], HANDLE_SIZE);
        strncpy(buf, handle, HANDLE_SIZE);

        ioctl(fd, IOCTL_LOGIN, handle);

        sleep(5);
        strncpy(buf + HANDLE_SIZE, "asdfgh", MESSAGE_SIZE);
        write(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        sleep(5);
        read(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        printf("%s %s\n", buf, buf + HANDLE_SIZE);


        ioctl(fd, IOCTL_LOGOUT, handle);
        close(fd);
        return 0;
}
