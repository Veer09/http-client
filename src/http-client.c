#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "../include/utils.h"
#include "../include/http-client.h"

ReturnCode get_addr_info(const Url* url, struct addrinfo** addr){
    int status;
    struct addrinfo hint;
    struct addrinfo* res;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC; // For any type of address family
    hint.ai_socktype = SOCK_STREAM; // For TCP connection
    //DNS lookup    
    if((status = getaddrinfo(url->host, url->port, &hint, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return ERR_SYSTEM_ERROR;
    }
    *addr = res;
    return SUCCESS;
}


ReturnCode send_request(struct addrinfo* addr, const char* request){
    struct addrinfo* p;
    int socket_fd;
    for(p = addr; p != NULL; p = p->ai_next){
        if((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("Can't create socket!!");
            continue;
        }
        if(connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(socket_fd);
            perror("Can't connect to server!!");
            continue;
        }
        break;
    }

    if(p == NULL){
        perror("Can't connect to server!!");
        close(socket_fd);
        return ERR_SYSTEM_ERROR;
    }

    if((send(socket_fd, request, strlen(request), 0)) == -1){
        close(socket_fd);
        perror("Can't send request!!");
        return ERR_SYSTEM_ERROR;
    }
    char* response = (char*)malloc(BUFSIZ);
    char buff[BUFSIZ];
    int recv_len;
    int total_len = 0;
    while((recv_len = recv(socket_fd, buff, BUFSIZ-1, 0)) > 0){
        buff[recv_len] = '\0';
        if(total_len + recv_len >= strlen(response)){
            response = (char*)realloc(response, total_len + recv_len + 1);
            if(response == NULL){
                close(socket_fd);
                return ERR_MEMORY_ALLOCATION;
            }
        }
        memcpy(response + total_len, buff, recv_len);
        total_len += recv_len;
    }

    if(recv_len < 0){
        perror("Can't receive response!!");
        return ERR_SYSTEM_ERROR;
    }
    close(socket_fd);

    response = (char*) realloc(response, strlen(response) + 1);
    if(response == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    printf("%s\n", response);
    free(response);
    return SUCCESS;
}
