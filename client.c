#include "client.h"

void init_socket(int* sockfd, struct hostent** he, struct sockaddr_in* their_addr) {
    if (((*he) = gethostbyname(IP)) == NULL) {  /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset(their_addr, 0, sizeof(struct sockaddr_in));
    their_addr->sin_family = AF_INET;      /* host byte order */
    their_addr->sin_port = htons(PORT);    /* short, network byte order */
    their_addr->sin_addr = *((struct in_addr *)(*he)->h_addr);

    if (connect(*sockfd, (struct sockaddr *)their_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
}

int main()
{
    int sockfd = 0;
    int numbytes = 0; 
    char response[32] = {0};
    char buffer[MAXDATASIZE] = { 0 };
    struct hostent *he = NULL;
    struct sockaddr_in their_addr = { 0 }; /* connector's address information */
    int temp = 0;
    mem_dump_request request = {0};

    init_socket(&sockfd, &he, &their_addr);

    while (1) {
	   printf(">>");
       memset(buffer, 0, MAXDATASIZE);
	   fgets(buffer, MAXDATASIZE, stdin);

	   send(sockfd, buffer, strlen(buffer), 0);
       recv(sockfd, response, 32, 0);
       
       if(strcmp(response, "Invalid input.") == 0)
            continue;
       
       memset(buffer, 0, MAXDATASIZE);

       temp = recv(sockfd,buffer,20,0);
       sscanf(buffer,"%10p %08x",&request.starting_address,&request.length);

       memset(buffer, 0, MAXDATASIZE);

       temp = recv(sockfd,buffer,request.length,0);
       print_mem(request.starting_address, request.length, buffer);
    }

    close(sockfd);

    return 0;
}
