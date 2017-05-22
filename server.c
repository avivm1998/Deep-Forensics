#include "server.h"

/* Initializing the netlink socket to connect to the kernel */
void init_netlink_socket(int* nl_fd, struct sockaddr_nl* src_addr, struct sockaddr_nl* dest_addr, struct nlmsghdr** nlh, 
	struct nlmsghdr** nlh_in, struct iovec* iov, struct iovec* iov_in, struct msghdr* msg, struct msghdr* msg_in) {

	int optval = 1;

	/*memset(src_addr, 0, sizeof(*src_addr));
	memset(dest_addr, 0, sizeof(*dest_addr));
	memset(iov, 0, sizeof(*iov));
	memset(msg, 0, sizeof(*msg));*/

	if ((*nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER)) < 0) {
		perror("netlink socket");
		exit(1);
	}

	//memset(src_addr, 0, sizeof(*src_addr));
	src_addr->nl_family = AF_NETLINK;
	src_addr->nl_pid = getpid(); /* self pid */ 
	src_addr->nl_groups = 0; /* not in mcast group */

	if(bind(*nl_fd, (struct sockaddr *)src_addr, sizeof(*src_addr)) < 0) {
		perror("netlink bind");
		exit(1);
	}

	//memset(dest_addr, 0, sizeof(*dest_addr));
	dest_addr->nl_family = AF_NETLINK;
	dest_addr->nl_pid = 0; /* Linux kernel */
	dest_addr->nl_groups = 0; /* unicast */
	
    setsockopt(*nl_fd, SOL_SOCKET, NETLINK_NO_ENOBUFS, &optval, sizeof(optval));

	*nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(*nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    (*nlh)->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    (*nlh)->nlmsg_pid = getpid();
    (*nlh)->nlmsg_flags = 0;

    /*iov->iov_base = (void *)(*nlh);
    iov->iov_len = (*nlh)->nlmsg_len;
    msg->msg_name = (void *)dest_addr;
    msg->msg_namelen = sizeof(*dest_addr);
    msg->msg_iov = iov;
    msg->msg_iovlen = 1;*/
                                            
    /**nlh_in = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(*nlh_in, 0, NLMSG_SPACE(MAX_PAYLOAD));
    (*nlh_in)->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    (*nlh_in)->nlmsg_pid = getpid();
    (*nlh_in)->nlmsg_flags = 0;

    iov_in->iov_base = (void *)(*nlh_in);
    iov_in->iov_len = (*nlh_in)->nlmsg_len;
    msg_in->msg_iov = iov_in;
    msg_in->msg_iovlen = 1;*/
}

/* Initializing the socket that connects to the client */
void init_tcp_socket(int* sock_fd, int* client_fd, struct sockaddr_in* addr, struct sockaddr_in* client) {
	int optval = 1;
	int size = 0;

	if((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("app socket");
		exit(1);
	}

    setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	memset(&(*addr), 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(PORT);

	if(bind(*sock_fd, (struct sockaddr *)addr, sizeof((*addr))) < 0) {
		perror("app bind");
		exit(1);
	}

	listen(*sock_fd, 1);

	size = sizeof(struct sockaddr_in);    	
    if((*client_fd = accept(*sock_fd, (struct sockaddr *) client, (socklen_t*) &size)) < 0) {
       	perror("app accept");
       	exit(1);
    }
}

int main(int argc,char** argv)
{
	int sock_fd = 0;
	int client_fd = 0;
	struct sockaddr_in addr = { 0 }; 
	struct sockaddr_in client = { 0 };
	int size = 0;
	int read_bytes = 0;
	char buffer[MAX_PAYLOAD] = { 0 };

	int res;

	int nl_fd = 0;
	struct sockaddr_nl src_addr = { 0 };
	struct sockaddr_nl dest_addr = { 0 };
	struct nlmsghdr *nlh = NULL;
	struct nlmsghdr *nlh_in = NULL;
	struct iovec iov = { 0 };
	struct iovec iov_in = { 0 };
	struct msghdr msg = { 0 };
	struct msghdr msg_in = { 0 };
	int i,j,k;
	
	init_netlink_socket(&nl_fd, &src_addr, &dest_addr, &nlh, &nlh_in, &iov, &iov_in, &msg, &msg_in);
	init_tcp_socket(&sock_fd, &client_fd, &addr, &client);

    read_bytes = recv(client_fd, buffer, MAX_PAYLOAD, 0);

	strcpy(NLMSG_DATA(nlh), "0x0000,0x0100");


	iov.iov_base = (void *)nlh;
	iov.iov_len = nlh->nlmsg_len;
	msg.msg_name = (void *)&dest_addr;
	msg.msg_namelen = sizeof(dest_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	printf("Sending message to kernel\n");
	res = sendmsg(nl_fd,&msg, 0);
	if(res < 0) {
		perror("sendmsg");
		return -1;
	}
	printf("Waiting for message from kernel\n");

	/* Read message from kernel */
	res = recvmsg(nl_fd, &msg, 0);
	if(res < 0) {
		perror("recvmsg");
		return -1;
	}

	for (i = 0; i < res; ++i)
	{
		buffer[i] = ((char*)NLMSG_DATA(nlh))[i];
	}

	printf("Address:\tData:\n");
	for (i = 0; i < res; i+=16)
	{
		printf("%08x\t",i);
		for (j = 0; j < 2; ++j)
		{
			for (k = 0; k < 8; ++k)
			{
				printf("%02x|",buffer[i+j*8+k]&0xff);
			}
			printf("\t");
		}
		printf("\n");
	}
	close(nl_fd);

	return 0;

}
