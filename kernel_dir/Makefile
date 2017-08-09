obj-m += kernel_module.o



all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir modules
	mv kernel_module.ko ../kernel_module.ko
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir clean
	rm ../kernel_module.ko
