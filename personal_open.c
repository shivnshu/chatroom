#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "chatroom.h"

char buf[2*HANDLE_SIZE + MESSAGE_SIZE];
char handle[HANDLE_SIZE];
char recv_handle[HANDLE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("Usage: ./bin <handle> [<recv_handle>]");
                return 0;
        } else if (argc == 2) {
                strcpy(recv_handle, "\0");
        } else if (argc == 3) {
                strncpy(recv_handle, argv[2], HANDLE_SIZE);
        } else {
                printf("No. of args are more than 3 \n");
                return 0;
        }
  
        strncpy(handle, argv[1], HANDLE_SIZE);

        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        strcpy(buf, handle);
        strcpy(buf+HANDLE_SIZE, recv_handle);

        ioctl(fd, IOCTL_LOGIN, handle);

        sleep(1);
        strncpy(buf+2*HANDLE_SIZE, "asdfgh", MESSAGE_SIZE);
        write(fd, buf, 2*HANDLE_SIZE+MESSAGE_SIZE);
        sleep(1);
        strncpy(buf+2*HANDLE_SIZE, "", MESSAGE_SIZE);
        read(fd, buf, 2*HANDLE_SIZE+MESSAGE_SIZE);
        printf("%s %s %s\n", buf, buf+HANDLE_SIZE, buf+2*HANDLE_SIZE);


        ioctl(fd, IOCTL_LOGOUT, handle);
        close(fd);
        return 0;
}
