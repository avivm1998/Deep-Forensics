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
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    char response[32];
    struct hostent *he;
    struct sockaddr_in their_addr; /* connector's address information */

    init_socket(&sockfd, &he, &their_addr);

    while (1) {
	   printf("Enter string: ");
	   fgets(buf, MAXDATASIZE, stdin);

	   send(sockfd, buf, strlen(buf), 0);
       recv(sockfd, response, 32, 0);

       printf("Message from the server: %s\n", response);
    }

    close(sockfd);

    return 0;
}
