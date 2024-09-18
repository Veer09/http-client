#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/utils.h"

const char* get_error_message(int code){
    switch(code){
        case ERR_INVALID_ARGS:
            return "Invalid Arguments!!";
        case ERR_INVALID_RESPONSE:
            return "Invalid response!!";
        case ERR_MEMORY_ALLOCATION:
            return "Can't allocate memory!!";
        case ERR_UNKNOWN:
            return "Unknown error!!";
        case ERR_INVALID_IPv6:
            return "Invalid IPv6 address!!";
        case ERR_INVALID_URL:
            return "Invalid URL!!";
        case ERR_INVALID_METHOD:
            return "Invalid method!!";
        case ERR_INVALID_HEADER:
            return "Invalid header!!";
        case ERR_INVALID_PROTOCOL:
            return "Invalid protocol!!";
        case ERR_SYSTEM_ERROR:
            return "";
    }
    return "Something went wrong!!";
}

void handle_error(ReturnCode code){
    fprintf(stderr, "Error: %s\n", get_error_message(code));
}

ReturnCode parse_url(const char *url, Request* request) {
    Url* p_url = (Url*) malloc(sizeof (Url));
    if(p_url == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    p_url->protocol = NULL;
    p_url->host = NULL;
    p_url->port = NULL;
    p_url->path = NULL;
    
    // Find the position of the "://"
    const char *protocol_end = strstr(url, "://");
    if (protocol_end != NULL) {
        // Extract the protocol
        size_t protocol_length = protocol_end - url;
        p_url->protocol = (char*) malloc(protocol_length+1);
        if(p_url->protocol == NULL){
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->protocol, url, protocol_length);
        p_url->protocol[protocol_length] = '\0';

        // Move past "://"
        url = protocol_end + 3;
    }else{
        p_url->protocol = (char*)malloc(5);
        if(p_url->protocol == NULL){
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strcpy(p_url->protocol, "http");
    }
    // Find the position of the first '/'
    const char *path_start = strchr(url, '/');
    if (path_start == NULL) {
        p_url->path = (char*)malloc(2);
        if(p_url->path == NULL){
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strcpy(p_url->path, "/");
        path_start = url + strlen(url);
    }
    else{
        // Extract the path
        size_t path_length = strlen(path_start);
        p_url->path = (char*)malloc(path_length+1);
        if(p_url->path == NULL){
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->path, path_start, path_length);
        p_url->path[path_length] = '\0';
    }

    // Extract the host and port from the part before the path
    size_t host_length = path_start - url;
    char host_and_port[host_length + 1];
    strncpy(host_and_port, url, host_length);
    host_and_port[host_length] = '\0';

    // Handle IPv6 addresses enclosed in square brackets
    char *ipv6_start = strchr(host_and_port, '[');
    if (ipv6_start != NULL) {
        char *ipv6_end = strchr(ipv6_start, ']');
        if (ipv6_end == NULL) {
            free_url(p_url);
            return ERR_INVALID_IPv6;
        }
        size_t ipv6_length = ipv6_end - ipv6_start - 1;
        p_url->host = (char*) malloc(ipv6_length+1);
        if(p_url->host == NULL){
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->host, ipv6_start + 1, ipv6_length);
        p_url->host[ipv6_length] = '\0';

        // Check for a port after the closing bracket
        char *port_start = ipv6_end + 1;
        if (*port_start == ':') {
            size_t port_length = path_start - port_start - 1;
            p_url->port = (char*) malloc(port_length+1);
            if(p_url->port == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->port, port_start+1, port_length);
            p_url->port[port_length] = '\0';
        } else {
            p_url->port = (char*) malloc(3);
            if(p_url->port == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->port, "80");
        }
    } else {
        // Find the position of the ':'
        char *port_start = strchr(host_and_port, ':');
        if (port_start == NULL) {
            // No port specified
            p_url->host = (char*) malloc(host_length+1);
            if(p_url->host == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->host, host_and_port);
            p_url->port = (char*) malloc(3);
            if(p_url->port == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->port, "80");
        } else {
            // Extract the host

            size_t host_size = port_start - host_and_port;
            p_url->host = (char*) malloc(host_size+1); 
            if(p_url->host == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->host, host_and_port, host_size);
            p_url->host[host_size] = '\0';

            // Extract the port
            size_t port_length = path_start - port_start - 1;
            p_url->port = (char*) malloc(port_length+1);
            if(p_url->port == NULL){
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->port, port_start+1, port_length);
            p_url->port[port_length] = '\0';
        }
    }
    request->url = p_url;
    return SUCCESS;
}

void free_url(Url* url) {
    if(url == NULL) return;
    if(url->protocol) free(url->protocol);
    if(url->host) free(url->host);
    if(url->port) free(url->port);
    if(url->path) free(url->path);
    free(url);
}

//-m for method, -h for header, -b for body, -p for port

ReturnCode parse_args(int argc, char* argv[], Request* req){
    if(argc < 2){
        return ERR_INVALID_ARGS;
    }
    for(int i=1;i<argc;i++){
        char* arg = argv[i];
        // For flag checking
        if(arg[0] == '-'){
            if(strlen(arg) != 2){
                return ERR_INVALID_ARGS;
            }
            char* nextArg = argv[i+1];
            ReturnCode res = get_args(arg, nextArg, req);
            if(res) return res;
            i++;
        }else{
            ReturnCode res = parse_url(arg, req);
            if(res) return res;
        } 
    }
    ReturnCode res = add_default_headers(req);
    if(res) return res;
    return SUCCESS;
}

ReturnCode init_request(Request** req){
    Request* t_req = (Request*) malloc(sizeof(Request));
    if(t_req == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    t_req->method = GET;
    t_req->url = NULL;
    t_req->headers = (HeaderList*) malloc(sizeof(HeaderList));
    if(t_req->headers == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    t_req->headers->head = NULL;
    t_req->headers->tail = NULL;
    t_req->headers->size = 0;
    t_req->body = NULL;
    *req = t_req;
    return SUCCESS;
}

ReturnCode get_args(char* flag, char* value, Request* request){
    switch(flag[1]){
        case 'm':
            if(strcmp(value, "GET") == 0){
                request->method = GET;
            }else if(strcmp(value, "POST") == 0){
                request->method = POST;
            }else if(strcmp(value, "PUT") == 0){
                request->method = PUT;
            }else if(strcmp(value, "DELETE") == 0){
                request->method = DELETE;
            }else{
                return ERR_INVALID_METHOD;
            }
            break;
        case 'h':
            char* key = strtok(value, ":");
            char* val = strtok(NULL, ":");
            if(key == NULL || val == NULL){
                return ERR_INVALID_HEADER;
            }
            ReturnCode res = add_header(key, val, request);
            if(res){
                return res;
            }
            break;
        case 'b':
            request->body = (char*) malloc(strlen(value) + 1);
            if(request->body == NULL){
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(request->body, value);
            break;
        case 'p':
            request->url->port = (char*) malloc(strlen(value) + 1);
            if(request->url->port == NULL){
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(request->url->port, value);
            break;
        default:
            return ERR_INVALID_ARGS;
    }
    return SUCCESS;
}

bool in_list(char* key, HeaderList* headers){
    Header* header = headers->head;
    for(int i=0;i<headers->size;i++){
        if(strcmp(key, header->key) == 0){
            return true;
        }
        header = header->next;
    }
    return false;
}

ReturnCode add_default_headers(Request* request){
    if(!in_list("Host", request->headers)){
        ReturnCode res = add_header("Host", request->url->host, request);
        if(res){
            return res;
        }
    }
    if(!in_list("Accept", request->headers)){
        ReturnCode res = add_header("Accept", "*/*", request);
        if(res){
            return res;
        }
    }
    if(!in_list("Connection", request->headers)){
        ReturnCode res = add_header("Connection", "close", request);
        if(res){
            return res;
        }
    }
    return SUCCESS;
}

ReturnCode add_header(char* key, char* value, Request* request){
    Header* header = malloc(sizeof(struct Header));
    if(header == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    header->key = key;
    header->value = value;

    if(request->headers->size == 0){
        request->headers->head = header;
        request->headers->tail = header;
    }else{
        request->headers->tail->next = header;
        request->headers->tail = header;
    }
    request->headers->size++;
    return SUCCESS;
}



// struct request* create_request(const char* method, const char* header, const Url* url){
//     struct request* req = (struct request*) malloc(sizeof(struct request));
//     req->method = (char*) malloc(strlen(method) + 1);
//     strcpy(req->method, method);
//     req->path = (char*) malloc(strlen(url->path) + 1);
//     strcpy(req->path, url->path);
//     req->host = (char*) malloc(strlen(url->host) + 1);
//     strcpy(req->host, url->host);
//     req->port = atoi(url->port);
//     req->header = (char*) malloc(strlen(header) + 1);
//     strcpy(req->header, header);
//     return req;
// }

char* request_to_string(const Request* req){
    char* request = malloc(1024);
    if(request == NULL){
        return NULL;
    }
    snprintf(request, 1024, "%s %s HTTP/1.1\r\n", "GET", req->url->path);
    Header* header = req->headers->head;
    for(int i=0;i<req->headers->size;i++){
        strcat(request, header->key);
        strcat(request, ": ");
        strcat(request, header->value);
        strcat(request, "\r\n");
        header = header->next;
    }
    strcat(request, "\r\n");
    if(req->body != NULL){
        strcat(request, req->body);
    }
    return request;
}