#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include<time.h>

#include "chatroom.h"

char buf[HANDLE_SIZE + MESSAGE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage: ./bin <handle>");
                return 0;
        }
        int i;
        char msg[MESSAGE_SIZE];
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
        snprintf(msg, MESSAGE_SIZE, "Public message by %s at timestamp %lu", handle, (unsigned long)time(NULL));
        strncpy(buf + HANDLE_SIZE, msg, MESSAGE_SIZE);
        write(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        sleep(5);
        read(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        printf("%s\n", buf + HANDLE_SIZE);


        ioctl(fd, IOCTL_LOGOUT, handle);
        close(fd);
        return 0;
}
