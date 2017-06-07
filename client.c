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

void print_mem(uintptr_t starting_address, int length, unsigned char* data) {
    int i = 0;            
    int j = 0;
                
    for (i = 0; i < length;) {     
        printf("%10p\t", starting_address + i);
        
        for(j = 0; j < 8; j++) {
            if(i + j < length)
                printf("%02x ",data[i+j]);
            else
                printf(".. ");
        }
        
        printf("\t");
        
        for(j = 0; j < 8; j++) {
            if(i+j < length) {
                if(isprint(data[i+j]))
                    printf("%c",data[i+j]);
                                                                                                else
                    printf(".");
            }
            
            else
                printf(".");
        }
        
        printf("\n");
        i += 8;
    }
}

int main()
{
    int sockfd = 0;
    int numbytes = 0; 
    uintptr_t starting_address = 0;
    int length = 0;
    char response[32] = {0};
    char buffer[MAXDATASIZE] = { 0 };
    struct hostent *he = NULL;
    struct sockaddr_in their_addr = { 0 }; /* connector's address information */

    init_socket(&sockfd, &he, &their_addr);

    while (1) {
	   printf(">");
	   fgets(buffer, MAXDATASIZE, stdin);

	   send(sockfd, buffer, strlen(buffer), 0);
       recv(sockfd, response, 32, 0);
       
       if(strcmp(response, "Invalid input.") == 0)
            continue;
       
       memset(buffer, 0, MAXDATASIZE);

       recv(sockfd, &starting_address, sizeof(starting_address), 0 );
       recv(sockfd, &length, sizeof(length), 0);
       recv(sockfd, buffer, MAXDATASIZE, 0);
        
       printf("%010p, %08x\n", starting_address, length);

       print_mem(starting_address, length, buffer);
       
    }

    close(sockfd);

    return 0;
}
