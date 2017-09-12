#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include <time.h>

#include "../chatroom.h"

char buf[HANDLE_SIZE + MESSAGE_SIZE];
char handle_credential[HANDLE_SIZE + IDENTIFIER_SIZE];
char handles[MAX_ONLINE_PROCESSES][HANDLE_SIZE];
char recv_handle[HANDLE_SIZE];

int main(int argc, char **argv)
{
        if (argc < 3) {
                printf("Usage: ./bin <handle> <handle_identifier> [<recv_handle>]");
                return 0;
        } else if (argc == 3) {
                strcpy(recv_handle, "\0");
        } else if (argc == 4) {
                strncpy(recv_handle, argv[3], HANDLE_SIZE);
        } else {
                printf("No. of args are more than 3 \n");
                return 0;
        }
  
        strncpy(handle_credential, argv[1], HANDLE_SIZE);
        strncpy(handle_credential + HANDLE_SIZE, argv[2], IDENTIFIER_SIZE);

        int i;
        int fd = open("/dev/chatroom", O_RDWR);
        char msg[MESSAGE_SIZE];
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        if (ioctl(fd, IOCTL_LOGIN, handle_credential) < 0) {
                printf("Wrong credentials\n");
                exit(0);
        }

        if (ioctl(fd, IOCTL_CHECKLOGIN, handles) < 0) {
                perror("ioctl");
        }

        strcpy(buf, recv_handle);

        int flag = 0;
        for (i=0;i<MAX_ONLINE_PROCESSES;++i) {
                if (!strcmp(recv_handle, handles[i])) {
                                flag = 1;
                                break;
                }
        }

        if (!flag && strlen(recv_handle)) {
                printf("%s is not online\n", recv_handle);
                ioctl(fd, IOCTL_LOGOUT, NULL);
                close(fd);
                return 0;
        }
                                

        if (!strlen(recv_handle))
                strcpy(recv_handle, "ALL");
        snprintf(buf + HANDLE_SIZE, MESSAGE_SIZE, "Message to %s by %s at timestamp %lu", recv_handle, handle_credential, (unsigned long)time(NULL));
        sleep(1);

        write(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        sleep(1);

        strcpy(buf, "");
        read(fd, buf, HANDLE_SIZE + MESSAGE_SIZE);
        printf("%s\n", buf);


        ioctl(fd, IOCTL_LOGOUT, NULL);
        close(fd);
        return 0;
}
