#include "kernel_module.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>

#include <net/sock.h>

#define PORT 1202
#define MAX_SIZE 256

struct service {
    struct socket *listen_socket;
    struct task_struct *thread;
};

struct service *svc;

int recv_msg(struct socket *sock, void *buf, int len) 
{
    struct msghdr msg;
    struct kvec iov;
    int size = 0;

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg.msg_name = 0;
    msg.msg_namelen = 0;

    size = kernel_recvmsg(sock, &msg, &iov, 1, len, msg.msg_flags);

    if (size > 0)
        printk(KERN_ALERT "the message is : %d %d\n",((int*)buf)[0],((int*)buf)[1]);

    return size;
}

int send_msg(struct socket *sock,void *buf,int len) 
{
    struct msghdr msg;
    struct kvec iov;
    int size;
    int j;

    iov.iov_base = buf;
    iov.iov_len = len;

    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    msg.msg_name = 0;
    msg.msg_namelen = 0;

    size = kernel_sendmsg(sock, &msg, &iov, 1, len);

    if (size > 0)
        for(j = 0; j<len;j++){
            printk(KERN_INFO "%d %x\n",j,((int*)buf)[j]);
        }
        printk(KERN_INFO "message sent!\n");

    return size;
}

int start_listen(void)
{
    struct socket *acsock;
    int error, i, size;
    struct sockaddr_in sin;
    int buf[2]={0};
    char data[MAX_SIZE]={0};

    error = sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP,
            &svc->listen_socket);
    if(error<0) {
        printk(KERN_ERR "cannot create socket\n");
        return -1;
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    error = kernel_bind(svc->listen_socket, (struct sockaddr*)&sin,
            sizeof(sin));
    if(error < 0) {
        printk(KERN_ERR "cannot bind socket, error code: %d\n", error);
        return -1;
    }

    error = kernel_listen(svc->listen_socket,5);
    if(error<0) {
        printk(KERN_ERR "cannot listen, error code: %d\n", error);
        return -1;
    }

    i = 0;
    while (1) {
        error = kernel_accept(svc->listen_socket, &acsock, 0);
        if(error<0) {
            printk(KERN_ERR "cannot accept socket\n");
            return -1;
        }
        printk(KERN_ERR "sock %d accepted\n", i++);

        memset(&buf, 0, 2*sizeof(int));
        while ((size = recv_msg(acsock, buf, 2*sizeof(int))) > 0) {
            memset(&data, 0, MAX_SIZE);
            copy_data_from_memory(buf[0],buf[1],data,MAX_SIZE);
            send_msg(acsock, data, buf[1]);
            memset(&buf, 0, 2*sizeof(int));
        }

        sock_release(acsock);
    }

    return 0;
}

static int __init mod_init(void)
{
    svc = kmalloc(sizeof(struct service), GFP_KERNEL);
    svc->thread = kthread_run((void *)start_listen, NULL, "echo-serv");
    printk(KERN_ALERT "echo-serv module loaded\n");

        return 0;
}

static void __exit mod_exit(void)
{
    if (svc->listen_socket != NULL) {
        kernel_sock_shutdown(svc->listen_socket, SHUT_RDWR);
        sock_release(svc->listen_socket);
        printk(KERN_ALERT "release socket\n");
    }
    
    kfree(svc);
    printk(KERN_ALERT "removed echo-serv module\n");
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

module_init(mod_init);
module_exit(mod_exit);
MODULE_DESCRIPTION("TCP server in the kernel");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dor Edelstein");
