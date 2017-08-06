obj-m += kernel_module.o



all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir modules

	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir clean
