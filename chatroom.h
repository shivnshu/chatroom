#ifndef __CHATROOM_H_
#define __CHATROOM_H_

#define DEV_MAJOR 243
#define MAX_ONLINE_PROCESSES 16
#define HANDLE_SIZE 32 
#define MESSAGE_SIZE 128
#define IDENTIFIER_SIZE 32

#define IOCTL_LOGIN _IOW(DEV_MAJOR, 0, char *)
#define IOCTL_LOGOUT _IOR(DEV_MAJOR, 1, char *)
#define IOCTL_CHECKLOGIN _IOR(DEV_MAJOR, 2, char **)
#define IOCTL_NUMMSG _IOR(DEV_MAJOR, 3, char *)
#define IOCTL_CA_REGISTER _IOW(DEV_MAJOR, 4, char *)

#endif
