#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include "utilities.h"

#define PORT 1202    /* the port client will be connecting to */
#define MAXDATASIZE 100 /* max number of bytes we can get at once */
#define IP "127.0.0.1"
#define INVALID_INPUT "invalid input"
#define EXIT "exit"
#define MEM_DUMP "d"


int main();
void init_socket(int* sockfd, struct hostent** he, struct sockaddr_in* their_addr);
