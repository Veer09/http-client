#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include "../include/utils.h"
#include "../include/http-client.h"

int main(int argc, char *argv[])
{
    struct addrinfo *addr = NULL;

    Global* global = (Global *)malloc(sizeof(global));
    ReturnCode rs;
    global->request = NULL;
    global->is_verbose = false;

    rs = init_request(global);
    if (rs != SUCCESS)
    {
        handle_error(rs);
        return 1;
    }

    rs = parse_args(argc, argv, global);
    if (rs != SUCCESS)
    {
        handle_error(rs);
        return 1;
    }
    rs = get_addr_info(global->request->url, &addr);

    if(rs != SUCCESS){
        handle_error(rs);
        return 1;
    }
    
    char* request_str = request_to_string(global->request);
    send_request(addr, request_str, global);
    free(request_str);
    free_url(global->request->url);
    freeaddrinfo(addr);
    return 0;
}
