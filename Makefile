obj-m += chatroom.o

all:
	make -C ../linux-4.14.4-cs698z M=$(PWD) modules

clean:
	make -C ../linux-4.14.4-cs698z M=$(PWD) clean
