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
#include <stdint.h>
#include "dataStruct.h"

#define PORT 1202
#define MAX_PAYLOAD 1024
#define NETLINK_USER 31


int main(int argc,char** argv);
void init_netlink_socket(int* nl_fd, struct sockaddr_nl* src_addr, struct sockaddr_nl* dest_addr, struct nlmsghdr** nlh, 
	struct nlmsghdr** nlh_in, struct iovec* iov, struct iovec* iov_in, struct msghdr* msg, struct msghdr* msg_in);
void init_tcp_socket(int* sock_fd, int* client_fd, struct sockaddr_in* addr, struct sockaddr_in* client);
int split(char*** parts, const char* str, const char* delimiter);
char* parse_input(mem_dump_request* request, char* input);
