#include "server.h"

/* Initializing the netlink socket to connect to the kernel */
void init_netlink_socket(int* nl_fd, struct sockaddr_nl* src_addr, struct sockaddr_nl* dest_addr, struct nlmsghdr** nlh, 
	struct nlmsghdr** nlh_in, struct iovec* iov, struct iovec* iov_in, struct msghdr* msg, struct msghdr* msg_in) {

	int optval = 1;

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

char* send_netlink_request(mem_dump_request request) {
    int size = 0;
    int recv_bytes = 0;
    char buffer[MAX_PAYLOAD] = { 0 };

    int nl_fd = 0;
    struct sockaddr_nl src_addr = { 0 };
    struct sockaddr_nl dest_addr = { 0 };
    struct nlmsghdr *nlh = NULL;
    struct nlmsghdr *nlh_in = NULL;
    struct iovec iov = { 0 };
    struct iovec iov_in = { 0 };
    struct msghdr msg = { 0 };
    struct msghdr msg_in = { 0 };
    int i;

    init_netlink_socket(&nl_fd, &src_addr, &dest_addr, &nlh, &nlh_in, &iov, &iov_in, &msg, &msg_in);

    sprintf(buffer,"%08x,%08x", request.starting_address, request.length);
    strncpy(NLMSG_DATA(nlh), buffer, strlen(buffer) + 1);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if(sendmsg(nl_fd, &msg, 0) < 0) {
        perror("sendmsg");
        return NULL;
    }

    if(recvmsg(nl_fd, &msg, 0) < 0) {
        perror("recvmsg");
        return NULL;
    }

    close(nl_fd);

    return (char*)NLMSG_DATA(nlh);
}

int main(int argc,char** argv)
{
    int sock_fd = 0;
    int client_fd = 0;
    char* response = NULL;
    char* net_res = NULL;
    char* command = NULL;
    char* failing_function = NULL;
    struct sockaddr_in addr = { 0 }; 
    struct sockaddr_in client = { 0 };
    int read_bytes = 0;
    char buffer[MAX_PAYLOAD] = { 0 };
    int temp = 0;
    int parsed_request[2] = { 0 };
    mem_dump_request request;

	init_tcp_socket(&sock_fd, &client_fd, &addr, &client);

    while(1) {
        if(recv(client_fd, command, sizeof(command) - 1, 0) == -1) {
            failing_function = "recv";
            goto failure;
        }

        // Exit command
        if(strcmp(command, EXIT) == 0) 
            break;

        // Memory dump request
        else {
            if(recv(client_fd, parsed_request, sizeof(parsed_request), 0) == -1) {
        	    failing_function = "recv";
        	    goto failure;
            }

            request.starting_address = parsed_request[0];
            request.length = parsed_request[1];

            net_res = send_netlink_request(request);

            if(send(client_fd, net_res, strlen(net_res) + 1, 0) == -1) {
                failing_function = "send";
                goto failure;
            }
        }
    }
	
	close(client_fd);
    close(sock_fd);

	return 0;

failure:
	close(client_fd);
	close(sock_fd);
	perror(failing_function);
	exit(1);

}
