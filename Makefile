obj-m += public_chatroom/chatroom.o
obj-m += private_chatroom/personal_chatroom.o
obj-m += private_chatroom_with_ca/personal_chatroom_ca.o

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules

clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
