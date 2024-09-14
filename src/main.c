#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "../include/utils.h"
#include "../include/http-client.h"



int main(int argc, char *argv[])
{
    if(argc != 2){
        fprintf(stderr, "Usage: %s <url>\n", argv[0]);
        return 1;
    }
    struct url_info* url = NULL;
    struct addrinfo* addr = NULL;
    parse_url(argv[1], &url);
    get_addr_info(url, &addr);
    struct request* request = create_request("GET", "",url); //for sample purpose, header is empty
    char* request_str = request_to_string(request);
    send_request(addr, request_str);
    free_url(url);
    freeaddrinfo(addr);  
    return 0;
}

