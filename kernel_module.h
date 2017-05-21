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

/*
* Prototypes
*/

int init_module(void);
void cleanup_module(void);
static void nl_recv_msg(struct sk_buff *skb);

#define SUCCESS 0
#define DEBUG 1
#define NETLINK_USER 31
#define EMPTY_MESSAGE "NO_DATA"
#define SIZE 1024