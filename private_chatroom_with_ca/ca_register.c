#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>

#include "../chatroom.h"

char buf[HANDLE_SIZE + IDENTIFIER_SIZE];

int main(int argc, char **argv)
{
        if (argc < 3){
                printf("Usage: <bin> <handle> <identifier>");
                return 0;
        }

        strncpy(buf ,argv[1], HANDLE_SIZE);
        strncpy(buf + HANDLE_SIZE, argv[2], IDENTIFIER_SIZE);

        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        if (ioctl(fd, IOCTL_CA_REGISTER, buf) < 0) {
                perror("ioctl");
        }

        close(fd);

        return 0;
}
