#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "../include/utils.h"

void parse_url(const char *url, struct url_info** f_url) {
    struct url_info* p_url = (struct url_info*) malloc(sizeof (struct url_info));
    if(p_url == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return ;
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
            fprintf(stderr, "Memory allocation failed\n");
            return ;
        }
        strncpy(p_url->protocol, url, protocol_length);
        p_url->protocol[protocol_length] = '\0';

        // Move past "://"
        url = protocol_end + 3;
    }else{
        p_url->protocol = (char*)malloc(5);
        if(p_url->protocol == NULL){
            free_url(p_url);
            fprintf(stderr, "Memory allocation failed\n");
            return ;
        }
        strcpy(p_url->protocol, "http");
    }
    // Find the position of the first '/'
    const char *path_start = strchr(url, '/');
    if (path_start == NULL) {
        p_url->path = (char*)malloc(2);
        if(p_url->path == NULL){
            free_url(p_url);
            fprintf(stderr, "Memory allocation failed\n");
            return ;
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
            fprintf(stderr, "Memory allocation failed\n");
            return ;
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
            fprintf(stderr, "Invalid IPv6 address format\n");
            exit(1);
        }
        size_t ipv6_length = ipv6_end - ipv6_start - 1;
        p_url->host = (char*) malloc(ipv6_length+1);
        if(p_url->host == NULL){
            free_url(p_url);
            fprintf(stderr, "Memory allocation failed\n");
            return ;
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
                fprintf(stderr, "Memory allocation failed\n");
                return ;
            }
            strncpy(p_url->port, port_start+1, port_length);
            p_url->port[port_length] = '\0';
        } else {
            p_url->port = (char*) malloc(3);
            if(p_url->port == NULL){
                free_url(p_url);
                fprintf(stderr, "Memory allocation failed\n");
                return ;
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
                fprintf(stderr, "Memory allocation failed\n");
                return ;
            }
            strcpy(p_url->host, host_and_port);
            p_url->port = (char*) malloc(3);
            if(p_url->port == NULL){
                free_url(p_url);
                fprintf(stderr, "Memory allocation failed\n");
                return ;
            }
            strcpy(p_url->port, "80");
        } else {
            // Extract the host

            size_t host_size = port_start - host_and_port;
            p_url->host = (char*) malloc(host_size+1); 
            if(p_url->host == NULL){
                free_url(p_url);
                fprintf(stderr, "Memory allocation failed\n");
                return ;
            }
            strncpy(p_url->host, host_and_port, host_size);
            p_url->host[host_size] = '\0';

            // Extract the port
            size_t port_length = path_start - port_start - 1;
            p_url->port = (char*) malloc(port_length+1);
            if(p_url->port == NULL){
                free_url(p_url);
                fprintf(stderr, "Memory allocation failed\n");
                return ;
            }
            strncpy(p_url->port, port_start+1, port_length);
            p_url->port[port_length] = '\0';
        }
    }
    *f_url = p_url;
}

void free_url(struct url_info* url) {
    if(url->protocol) free(url->protocol);
    if(url->host) free(url->host);
    if(url->port) free(url->port);
    if(url->path) free(url->path);
    free(url);
}

struct request* create_request(const char* method, const char* header, const struct url_info* url){
    struct request* req = (struct request*) malloc(sizeof(struct request));
    req->method = (char*) malloc(strlen(method) + 1);
    strcpy(req->method, method);
    req->path = (char*) malloc(strlen(url->path) + 1);
    strcpy(req->path, url->path);
    req->host = (char*) malloc(strlen(url->host) + 1);
    strcpy(req->host, url->host);
    req->port = atoi(url->port);
    req->header = (char*) malloc(strlen(header) + 1);
    strcpy(req->header, header);
    return req;
}

char* request_to_string(const struct request* req){
    char* request = (char*) malloc(256);
    if(request == NULL){
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    snprintf(request, 256, "%s %s HTTP/1.1\r\n"
                            "Host: %s\r\n"
                            "Accept: */*\r\n"
                            "Connection: close\r\n"
                            "\r\n", req->method, req->path, req->host);
    return request;
}