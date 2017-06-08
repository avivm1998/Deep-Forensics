typedef struct {
	uintptr_t starting_address;
	int length;
} mem_dump_request;

void print_mem(uintptr_t starting_address, int length, unsigned char* data) {
    int i = 0;            
    int j = 0;
                
    for (i = 0; i < length;) {     
        printf("%010p\t", starting_address + i);
        
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

// Splits a given string with a given delimiter and fills the parts to the given (by address) 
// string array, The function also returns the size of the array.
// The array and all of its strings are dynamically allocated and should be freed after use.
int split(char*** parts, const char* str, const char* delimiter) {
    int i = 0;
    int counter = 0;
    char* token = NULL;
    char* buffer = NULL;
    char* ptr = NULL;

    buffer = strdup(str);
    ptr = buffer;
    size_t nlen = strlen(delimiter);

    while (ptr != NULL) {
	ptr = strstr (ptr, delimiter);

	if (ptr != NULL) {
	    counter++;
	    ptr += nlen;
	}
    }

    (*parts) = (char**)calloc(counter + 1, sizeof(char*));

    token = strtok(buffer, delimiter);

    i = 0;
    while(token != NULL) {
	(*parts)[i++] = strdup(token);
	token = strtok(NULL, delimiter);
    }

    free(buffer);

    return counter + 1;
}