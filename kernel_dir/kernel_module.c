#include "kernel_module.h"

/*
* Global variables are declared as static, so are global within the file.
*/

struct sock *nl_sk = NULL; /* The netlink socket */

/*
* This function is called when the module is loaded
*/
int init_module(void) {


    struct netlink_kernel_cfg cfg = {
        .input = nl_recv_msg,
    };

    #ifdef DEBUG
    printk(KERN_INFO "init module\n");
    #endif

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

    struct sk_buff* skb_out;
    struct nlmsghdr *nlh;
    char msg[256] = {0};
    int msg_size;
    int pid;
    int res;
    int i;

    int start = 0x0000;
    int length = 100;

    #ifdef DEBUG
        printk(KERN_INFO "nl_recv_msg\n");
    #endif

    msg_size = strlen(msg);
    memset(msg, 0, msg_size);

    nlh = (struct nlmsghdr *)skb->data;
    pid = nlh->nlmsg_pid;

    strcpy(msg, nlmsg_data(nlh));
    parse_input(msg, &start, &length);

    printk(KERN_INFO "%d %d\n",start,length);

    /* change the code here */
    msg_size = copy_data_from_memory(start, length, msg, 256);
    /* -------------------- */

    skb_out = nlmsg_new(msg_size, 0);
    if(!skb_out){
         printk(KERN_ERR "Error allocating message\n");
         return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;

    for (i = 0; i < msg_size; ++i)
    {
        ((char*)nlmsg_data(nlh))[i] = msg[i];
    }

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if(res < 0)
        printk(KERN_ERR "Error sending the message\n");

#ifdef DEBUG
    printk(KERN_INFO "------------");
    /*for (i = 0; i < length; i++) {
        printk(KERN_INFO "%d %x\n",i+1, ((char*)nlmsg_data(nlh))[i]);
    }*/
    printk(KERN_INFO "Netlink received msg payload: %s\n", (char*)nlmsg_data(nlh));
#endif
}

void parse_input(char* input, int* start, int* length){
    sscanf(input,"%08x,%08x",start,length);
}

int copy_data_from_memory(int start_address, int length, char* data, int buffer_length){
    void* ram_data;
    int count = 0;

    for (ram_data = 0; count < length && count < buffer_length; ram_data++ , count++)
    {
        if((long unsigned int)ram_data % (long unsigned int)PAGE_SIZE == 0){
            ram_data = phys_to_virt(start_address+count);
        }
        data[count] = *((char*)(ram_data)); 
    }

    return count;
}
