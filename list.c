#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>

#include "chatroom.h"

char handles[MAX_ONLINE_PROCESSES][HANDLE_SIZE];

int main()
{
        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        if (ioctl(fd, IOCTL_CHECKLOGIN, handles) < 0) {
                perror("ioctl");
        }

        for (i=0;i<MAX_ONLINE_PROCESSES;++i) {
                if (handles[i][0] != '\0')
                        printf("%s\n", handles[i]);
        }

        close(fd);

        return 0;
}
