# Deep-Forensics

Set up:
1. Download or clone our repository.
2. Open two terminals and traverse to the code directory.
3. Choose one of the terminals and use the make command.
4. Insert the kernel module using the command: sudo insmod kernel_module.ko
5. Start the server using the command: ./server
6. Open the other terminal and run the commmand: ./client

Usage:
Our current system works as follows:

CLIENT   <----- TCP ----->   SERVER   <----- NETLINK ----->   KERNEL


API:
1. d -a 0x1000 -l 256 - dumps 256 bytes (the -l parameter) of memory from the address 0x1000 (the -a parameter)
2. d -a 0x1000 -a 0x2000 - dump the memory between the address 0x1000 (the first -a parameter) and the address 0x2000 (the second -a parameter).
   
