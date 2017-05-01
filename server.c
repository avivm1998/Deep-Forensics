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
#define MAX_PAYLOAD 1024
#define NETLINK_USER 31



int main(int argc,char** argv)
{
	int sock_fd;
	int client_fd;
	struct sockaddr_in addr; 
	struct sockaddr_in client;
	int size;
	int read_bytes;
	char buffer[MAX_PAYLOAD];

	int nl_fd;
	struct sockaddr_nl src_addr;
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct nlmsghdr *nlh_in = NULL;
	struct iovec iov;
	struct iovec iov_in;
	struct msghdr msg;
	struct msghdr msg_in;

	/* Initializing the netlink socket to connect to the kernel */
	
	if ((nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER)) < 0) {
		perror("netlink socket");
		return -1;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); // self pid 

	if(bind(nl_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
		perror("netlink bind");
		return -1;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
                                            
    nlh_in = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    iov_in.iov_base = (void *)nlh_in;
    iov_in.iov_len = nlh->nlmsg_len;
    msg_in.msg_iov = &iov_in;
    msg_in.msg_iovlen = 1;                  

	/* ************************************************ */

	/* Initializing the socket that connects to the app */

	if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("app socket");
		return -1;
	}

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

	if(bind(sock_fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("app bind");
		return -1;
	}

	listen(sock_fd, 1);

	size = sizeof(struct sockaddr_in);    	
    if((client_fd = accept(sock_fd, (struct sockaddr *) &client, (socklen_t*) &size)) < 0) {
       	perror("app accept");
       	return -1;
    }

    /* *************************************************** */

	do {
		memset(buffer, 0, MAX_PAYLOAD);
        read_bytes = recv(client_fd, buffer, MAX_PAYLOAD, 0);
		strcpy(NLMSG_DATA(nlh), buffer);
		printf("%s\n", (char*)NLMSG_DATA(nlh));

		printf("Sending message to kernel\n");
		if(sendmsg(nl_fd, &msg, 0) < 0) {
			perror("sendmsg");
			return -1;
		}
		printf("Waiting for message from kernel\n");

		/* Read message from kernel */
		if(recvmsg(nl_fd, &msg_in, 0) < 0) {
			perror("recvmsg");
			return -1;
		}
		printf("Received message payload: %s\n", (char*)NLMSG_DATA(nlh_in));

	} while(read_bytes > 0);

	return 0;

}
