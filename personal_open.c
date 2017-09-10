#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "chatroom.h"

char buf[2*HANDLE_SIZE + MESSAGE_SIZE];
char handle[HANDLE_SIZE];
char toprocess_handle[HANDLE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 3) {
                printf("Usage: ./bin <handle> <toprocess_handle>");
                return 0;
        }
        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        strncpy(handle, argv[1], HANDLE_SIZE);
        strncpy(toprocess_handle, argv[2], HANDLE_SIZE);
        strncpy(buf, handle, HANDLE_SIZE);
        strncpy(buf+HANDLE_SIZE, toprocess_handle, HANDLE_SIZE);

        ioctl(fd, IOCTL_LOGIN, handle);

        sleep(1);
        strncpy(buf+2*HANDLE_SIZE, "asdfgh\n", MESSAGE_SIZE);
        write(fd, buf, 2*HANDLE_SIZE+MESSAGE_SIZE);
        sleep(1);
        read(fd, buf, 2*HANDLE_SIZE+MESSAGE_SIZE);
        printf("%s %s %s\n", buf, buf+HANDLE_SIZE, buf+2*HANDLE_SIZE);


        ioctl(fd, IOCTL_LOGOUT, handle);
        close(fd);
        return 0;
}
