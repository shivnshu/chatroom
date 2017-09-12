#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include<time.h>

#include "chatroom.h"

char buf[MESSAGE_SIZE];

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

        ioctl(fd, IOCTL_LOGIN, handle);

        sleep(5);
        snprintf(buf, MESSAGE_SIZE, "Public message by %s at timestamp %lu", handle, (unsigned long)time(NULL));
        write(fd, buf, MESSAGE_SIZE);
        sleep(5);
        read(fd, buf, MESSAGE_SIZE);
        printf("%s\n", buf);


        ioctl(fd, IOCTL_LOGOUT, NULL);
        close(fd);
        return 0;
}
