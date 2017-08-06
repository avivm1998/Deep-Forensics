all:
	make -C ./kernel_dir all
	make -C ./server_dir all
	make -C ./client_dir all 
	
clean:
	make -C ./kernel_dir clean
	make -C ./server_dir clean
	make -C ./client_dir clean 
