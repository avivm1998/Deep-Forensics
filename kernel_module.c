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
* Prototypes − this would normally go in a .h file
*/

int init_module(void);
void cleanup_module(void);
static void nl_recv_msg(struct sk_buff *skb);

#define SUCCESS 0
#define DEBUG 1
#define NETLINK_USER 31
#define EMPTY_MESSAGE "NO_DATA"

/*
* Global variables are declared as static, so are global within the file.
*/

struct sock *nl_sk = NULL; /* The netlink socket */

/*
* This function is called when the module is loaded
*/
int init_module(void) {
#ifdef DEBUG
    printk(KERN_INFO "init module\n");
#endif

    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -EFAULT;
    }

    return SUCCESS;
}

/*
* This function is called when the module is unloaded
*/
void cleanup_module(void) {
#ifdef DEBUG
    printk(KERN_INFO "cleanup module\n");
#endif

    netlink_kernel_release(nl_sk);

    printk(KERN_ALERT "Module has been removed\n");
    printk(KERN_INFO "---------------------------------------------------\n");
}

/*
* Methods
*/

static void nl_recv_msg(struct sk_buff *skb) {
#ifdef DEBUG
    printk(KERN_INFO "nl_recv_msg\n");
#endif
    struct sk_buff* skb_out;
    struct nlmsghdr *nlh;
    char* msg = "Hello from kernel";
    int msg_size;
    int pid;
    int res;

    msg_size = strlen(msg);

    memset(msg, 0, msg_size);

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;

    skb_out = nlmsg_new(msg_size, 0);
    if(!skb_out){
         printk(KERN_ERR "Error allocating message\n");
         return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;
    strncpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if(res < 0)
        printk(KERN_ERR "Error sending the message\n");

#ifdef DEBUG
    printk(KERN_INFO "Netlink received msg payload: %s\n", (char*)nlmsg_data(nlh));
#endif
}
