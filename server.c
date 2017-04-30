#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <unistd.h>

#define PORT 1202
#define SIZE 1024
#define NETLINK_USER 31

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd, nl_fd;
struct msghdr msg;

struct sockaddr_in addr,client;
char buffer[1024];
int fd_kern, client_sock, sock_fd, c, read_bytes;

int main(int argc,char** argv)
{
	/* Initializing the netlink socket to connect to the kernel */
	nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
	if (nl_fd < 0) {
		perror("netlink socket");
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); /* self pid */

	if(bind(nl_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
		perror("netlink bind");
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	dest_addr.nl_groups = 0; /* unicast */

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(SIZE));
	memset(nlh, 0, NLMSG_SPACE(SIZE));
	nlh->nlmsg_len = NLMSG_SPACE(SIZE);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	/* ************************************************ */

	/* Initializing the socket that connects to the app */
	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("app socket");
		return -1;
	}

    // set SO_REUSEADDR on a socket to true (1):
    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

	memset(&addr,0,sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

	if(bind(sock_fd,(struct sockaddr *)&addr,sizeof(addr)) < 0)
	{
		perror("app bind");
		return -1;
	}

	listen(sock_fd,1);

	c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client    	
    if((client_sock = accept(sock_fd, (struct sockaddr *)&client, (socklen_t*)&c)) < 0)
   	{
       	perror("app accept");
       	return -1;
    }
    /* *************************************************** */

	do {
		memset(buffer, 0, SIZE);
        memset(nlh, 0, NLMSG_SPACE(SIZE));

        read_bytes = recv(client_sock, buffer, SIZE, 0);
        printf("%s %d\n", buffer, read_bytes);
		strcpy(NLMSG_DATA(nlh), buffer);

		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_name = (void *)&dest_addr;
		msg.msg_namelen = sizeof(dest_addr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		if(sendmsg(nl_fd, &msg, 0) < 0) {
			perror("netlink sendmsg");
			return -1;
		}
        
        /* Read message from the kernel */
        if(recvmsg(nl_fd, &msg, 0) < 0) {
            perror("netlink recvmsg");
            return -1;
        }

        printf("%s\n", NLMSG_DATA(nlh));

	} while(1);
    
    close(client_sock);
    close(nl_fd);
	return 0;

}
