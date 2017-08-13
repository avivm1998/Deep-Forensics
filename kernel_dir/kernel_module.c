#include "kernel_module.h"

/*
* Global variables are declared as static, so are global within the file.
*/

struct netif netif;
ip4_addr_t ipaddr, netmask, gw;
struct task_struct *task;

/*
* This function is called when the module is loaded
*/
int init_module(void) {
#ifdef DEBUG
    printk(KERN_INFO "init module\n");
#endif

    init_lwip_server();

    task = kthread_run(&thread_function, NULL, "lwip_server");

    return SUCCESS;
}

/*
* This function is called when the module is unloaded
*/
void cleanup_module(void) {
#ifdef DEBUG
    printk(KERN_INFO "cleanup module\n");
#endif

    printk(KERN_ALERT "Module has been removed\n");
    printk(KERN_INFO "---------------------------------------------------\n");
}

/*
* Methods
*/

void init_lwip_server(void) {
    /* startup defaults (may be overridden by one or more opts) */
    IP4_ADDR(&gw, 192,168,0,1);
    IP4_ADDR(&ipaddr, 192,168,0,2);
    IP4_ADDR(&netmask, 255,255,255,0);

    lwip_init();

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    tcp_raw_init();
}

void thread_function(void* args) {
    while(!kthread_should_stop()) {
        /* poll netif, pass packet to lwIP */
        tapif_select(&netif);
        sys_check_timeouts();

        schedule();
    }
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
