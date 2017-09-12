obj-m += public_chatroom/chatroom.o
obj-m += private_chatroom/personal_chatroom.o
obj-m += private_chatroom_with_ca/personal_chatroom_ca.o

all:
	make -C ../linux-4.14.4-cs698z M=$(PWD) modules

clean:
	make -C ../linux-4.14.4-cs698z M=$(PWD) clean
