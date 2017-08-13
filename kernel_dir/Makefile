obj-m += kernel_module.o
kernel_module-obj += liblwipcommon.a liblwipapps.a


all:
	make -C ./minimal all
	mv ./minimal/liblwipcommon.a ./liblwipcommon.a
	mv ./minimal/liblwipapps.a ./liblwipapps.a
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir modules
	mv kernel_module.ko ../kernel_module.ko
	
clean:
	mv ./liblwipcommon.a ./minimal/liblwipcommon.a
	mv ./liblwipapps.a ./minimal/liblwipapps.a
	make -C ./minimal clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)/kernel_dir clean
	rm ../kernel_module.ko
	
