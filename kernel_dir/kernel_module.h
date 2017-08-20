/*
* Includes
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/random.h>
#include <asm/uaccess.h>    /* for put_user */
#include <linux/module.h>
#include <net/sock.h> 
#include <linux/netlink.h>
#include <linux/skbuff.h> 


#define SUCCESS 0
#define DEBUG 1
#define NETLINK_USER 31
#define EMPTY_MESSAGE "NO_DATA"
#define SIZE 1024

/*
* Prototypes
*/

int copy_data_from_memory(int start_address, int length, char* data, int buffer_length);
int split(char*** parts, const char* str, const char* delimiter);