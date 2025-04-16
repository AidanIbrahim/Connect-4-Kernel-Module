
obj-m += fourinarow.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc connect4user.c -o connect4

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm connect4

load:
	sudo insmod fourinarow.ko
unload:
	sudo rmmod fourinarow.ko

reload: 
	make
	make unload
	make load