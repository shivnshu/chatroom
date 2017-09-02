#ifndef __CHATROOM_H_
#define __CHATROOM_H_

#define DEV_MAJOR 243

#define IOCTL_LOGIN _IOW(DEV_MAJOR, 0, unsigned int *)
#define IOCTL_LOGOUT _IOR(DEV_MAJOR, 1, unsigned int)

#endif
