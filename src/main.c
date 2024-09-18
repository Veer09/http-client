#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "../include/utils.h"
#include "../include/http-client.h"

int main(int argc, char *argv[])
{
    struct addrinfo *addr = NULL;
    Request *req = NULL;
    ReturnCode rs;

    rs = init_request(&req);
    if (rs != SUCCESS)
    {
        handle_error(rs);
        return 1;
    }

    rs = parse_args(argc, argv, req);
    if (rs != SUCCESS)
    {
        handle_error(rs);
        return 1;
    }
    rs = get_addr_info(req->url, &addr);

    if(rs != SUCCESS){
        handle_error(rs);
        return 1;
    }
    for(Header* header = req->headers->head; header != NULL; header = header->next){
        printf("%s: %s\n", header->key, header->value);
    }
    char* request_str = request_to_string(req);
    send_request(addr, request_str);
    free(req->url);
    freeaddrinfo(addr);
    return 0;
}
