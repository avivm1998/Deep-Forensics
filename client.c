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

char* parse_input(mem_dump_request* request, char* input) {
	int temp = 0;
	int size = 0;
	int i = 0;
	char** parts = NULL;
	
	size = split(&parts, input, " ");
	
	request->starting_address = -1;
	request->length = -1;
	
	if(strcmp(parts[0], MEM_DUMP) == 0) {
		if(strcmp(parts[1], "-a") == 0) {
			if((temp = (int)strtol(parts[2], NULL, 0)) < 0)
				goto invalid_input;

			request->starting_address = temp;
			
			if(strcmp(parts[3], "-a") == 0) {
				if((temp = (int)strtol(parts[4], NULL, 0) - request->starting_address) <= 0)
					goto invalid_input;

				request->length = temp;
			}
			
			else if(strcmp(parts[3], "-l") == 0) {
				if((temp = atoi(parts[4])) <= 0)
					goto invalid_input;

				request->length = temp;
			}
			
			else {
				goto invalid_input;
			}
		}
		
		else {
			goto invalid_input;
		}
	}

	else if(strcmp(parts[0], EXIT) == 0) {
		return EXIT;
	}
	
	else {
		goto invalid_input;
	}
	
	return MEM_DUMP;

invalid_input:
	return INVALID_INPUT;
	
}

int main(void) {
    int sockfd = 0;
    int numbytes = 0; 
    char* command = NULL;
    char* failing_function = NULL;
    char buffer[MAXDATASIZE] = { 0 };
    struct hostent *he = NULL;
    struct sockaddr_in their_addr = { 0 }; /* connector's address information */
    mem_dump_request request = { 0 };
    int parsed_request[2] = { 0 };

    init_socket(&sockfd, &he, &their_addr);

    while (1) {
        printf(">>");
        memset(buffer, 0, MAXDATASIZE);
        fgets(buffer, MAXDATASIZE, stdin);

        buffer[strlen(buffer) - 1] = '\0'; //removing the \n

        command = parse_input(&request, buffer);
        
        parsed_request[0] = request.starting_address;
        parsed_request[1] = request.length;
        
        // in the case of an invalid command.
        if(strcmp(command, INVALID_INPUT) == 0) {
            printf("%s\n", command);
            continue;
        }

        else {
            if(send(sockfd, command, strlen(command) + 1, 0) == -1) {
                failing_function = "send";
                goto failure;
            }

            // if the command is exit break the loop.
            if(strcmp(command, EXIT) == 0)
                break;
    
           
            // otherwise continue with the memory dump request.
            else {
                if(send(sockfd, parsed_request , sizeof(parsed_request), 0) == -1) {
                    failing_function = "send";
                    goto failure;
                }

                memset(buffer, 0, MAXDATASIZE);
                if(recv(sockfd,buffer, request.length, 0) == -1) {
                    failing_function = "recv";
                    goto failure;
                }

                print_mem(request, buffer); 
            }
        }
    }

    close(sockfd);
    return 0;

failure:
    perror(failing_function);
    exit(1);

}
