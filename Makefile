obj-m += kernel_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc server.c -o server
	gcc client.c -o client
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf server server.o client client.o
