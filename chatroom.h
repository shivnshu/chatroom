#ifndef __CHATROOM_H_
#define __CHATROOM_H_

#define DEV_MAJOR 243

#define IOCTL_LOGIN _IOW(DEV_MAJOR, 0, char *)
#define IOCTL_LOGOUT _IOR(DEV_MAJOR, 1, char *)
#define IOCTL_CHECKLOGIN _IOR(DEV_MAJOR, 2, char *)

#endif