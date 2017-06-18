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

int main() {
  int sockfd = 0;
  int numbytes = 0; 
  char response[32] = {0};
  char* failing_function;
  char buffer[MAXDATASIZE] = { 0 };
  struct hostent *he = NULL;
  struct sockaddr_in their_addr = { 0 }; /* connector's address information */
  mem_dump_request request = {0};

  init_socket(&sockfd, &he, &their_addr);

  while (1) {
    printf(">>");
    memset(buffer, 0, MAXDATASIZE);
    fgets(buffer, MAXDATASIZE, stdin);

    buffer[strlen(buffer) - 1] = '\0'; //removing the \n

    if(send(sockfd, buffer, strlen(buffer), 0) == -1) {
      failing_function = "send";
      goto failure;
    }

    if(strcmp(buffer, EXIT) == 0)
      break;

    if(recv(sockfd, response, 32, 0) == -1) {
      failing_function = "recv";
      goto failure;
    }
      
    if(strcmp(response, INVALID_INPUT) == 0) {
      printf("%s\n", response);
      continue;
    }
     
    memset(buffer, 0, MAXDATASIZE);
    if(recv(sockfd, buffer, 20, 0) == -1) {
      failing_function = "recv";
      goto failure;
    }
    
    sscanf(buffer,"%10p %08x", &request.starting_address, &request.length);

    memset(buffer, 0, MAXDATASIZE);
    if(recv(sockfd,buffer, request.length, 0) == -1) {
      failing_function = "recv";
      goto failure;
    }

    print_mem(request, buffer);
  }

  close(sockfd);
  return 0;

failure:
  perror(failing_function);
  exit(1);

}
