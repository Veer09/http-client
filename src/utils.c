#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "../include/utils.h"

const char *get_error_message(int code)
{
    switch (code)
    {
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

void handle_error(ReturnCode code)
{
    if (code == ERR_SYSTEM_ERROR)
    {
        return;
    }
    fprintf(stderr, "Error: %s\n", get_error_message(code));
}

int is_number(const char *str)
{
    if (str == NULL || *str == '\0')
    {
        return 0;
    }
    char *endptr;
    long val = strtol(str, &endptr, 10);
    if (*endptr != '\0')
    {
        return 0;
    }
    return 1;
}

ReturnCode parse_url(const char *url, Global *global)
{
    Url *p_url = (Url *)malloc(sizeof(Url));
    if (p_url == NULL)
    {
        return ERR_MEMORY_ALLOCATION;
    }
    p_url->protocol = NULL;
    p_url->host = NULL;
    p_url->port = NULL;
    p_url->path = NULL;

    // Find the position of the "://"
    const char *protocol_end = strstr(url, "://");
    if (protocol_end != NULL)
    {
        // Extract the protocol
        size_t protocol_length = protocol_end - url;
        p_url->protocol = (char *)malloc(protocol_length + 1);
        if (p_url->protocol == NULL)
        {
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->protocol, url, protocol_length);
        p_url->protocol[protocol_length] = '\0';
        if (strcasecmp(p_url->protocol, "http") && strcasecmp(p_url->protocol, "https"))
        {
            free_url(p_url);
            return ERR_INVALID_URL;
        }
        // Move past "://"
        url = protocol_end + 3;
    }
    else
    {
        p_url->protocol = (char *)malloc(5);
        if (p_url->protocol == NULL)
        {
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strcpy(p_url->protocol, "http");
    }
    // Find the position of the first '/'
    const char *path_start = strchr(url, '/');
    if (path_start == NULL)
    {
        p_url->path = (char *)malloc(2);
        if (p_url->path == NULL)
        {
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strcpy(p_url->path, "/");
        path_start = url + strlen(url);
    }
    else
    {
        // Extract the path
        size_t path_length = strlen(path_start);
        p_url->path = (char *)malloc(path_length + 1);
        if (p_url->path == NULL)
        {
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->path, path_start, path_length);
        p_url->path[path_length] = '\0';
    }

    // Extract the host and port from the part before the path
    size_t host_and_port_length = path_start - url;
    char host_and_port[host_and_port_length + 1];
    strncpy(host_and_port, url, host_and_port_length);
    host_and_port[host_and_port_length] = '\0';

    // Handle IPv6 addresses enclosed in square brackets
    char *ipv6_start = strchr(host_and_port, '[');
    if (ipv6_start != NULL)
    {
        char *ipv6_end = strchr(ipv6_start, ']');
        if (ipv6_end == NULL)
        {
            free_url(p_url);
            return ERR_INVALID_IPv6;
        }
        size_t ipv6_length = ipv6_end - ipv6_start - 1;
        p_url->host = (char *)malloc(ipv6_length + 1);
        if (p_url->host == NULL)
        {
            free_url(p_url);
            return ERR_MEMORY_ALLOCATION;
        }
        strncpy(p_url->host, ipv6_start + 1, ipv6_length);
        p_url->host[ipv6_length] = '\0';

        // Check for a port after the closing bracket
        char *port_start = ipv6_end + 1;
        if (*port_start == ':')
        {
            size_t port_length = strlen(host_and_port) - ipv6_length - 1;
            p_url->port = (char *)malloc(port_length + 1);
            if (p_url->port == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->port, port_start + 1, port_length);
            p_url->port[port_length] = '\0';
            if (!is_number(p_url->port))
            {
                free_url(p_url);
                return ERR_INVALID_URL;
            }
        }
        else
        {
            p_url->port = (char *)malloc(3);
            if (p_url->port == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->port, "80");
        }
    }
    else
    {
        // Find the position of the ':'
        char *port_start = strchr(host_and_port, ':');
        if (port_start == NULL)
        {
            // No port specified
            p_url->host = (char *)malloc(host_and_port_length + 1);
            if (p_url->host == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->host, host_and_port);
            p_url->port = (char *)malloc(3);
            if (p_url->port == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strcpy(p_url->port, "80");
        }
        else
        {
            // Extract the host
            size_t host_size = port_start - host_and_port;
            p_url->host = (char *)malloc(host_size + 1);
            if (p_url->host == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->host, host_and_port, host_size);
            p_url->host[host_size] = '\0';

            // Extract the port
            size_t port_length = strlen(host_and_port) - host_size - 1;
            p_url->port = (char *)malloc(port_length + 1);
            if (p_url->port == NULL)
            {
                free_url(p_url);
                return ERR_MEMORY_ALLOCATION;
            }
            strncpy(p_url->port, port_start + 1, port_length);
            p_url->port[port_length] = '\0';
            if (!is_number(p_url->port))
            {
                free_url(p_url);
                return ERR_INVALID_URL;
            }
        }
    }
    global->request->url = p_url;
    return SUCCESS;
}

void free_url(Url *url)
{
    if (url == NULL)
        return;
    if (url->protocol)
        free(url->protocol);
    if (url->host)
        free(url->host);
    if (url->port)
        free(url->port);
    if (url->path)
        free(url->path);
    free(url);
}

//-m for method, -h for header, -b for body, -v verbose

ReturnCode parse_args(int argc, char *argv[], Global *global)
{
    if (argc < 2)
    {
        return ERR_INVALID_ARGS;
    }
    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        // For flag checking
        if (arg[0] == '-')
        {
            if (strlen(arg) != 2)
            {
                return ERR_INVALID_ARGS;
            }
            if (i + 1 > argc)
            {
                return ERR_INVALID_ARGS;
            }
            char *nextArg = argv[i + 1];
            ReturnCode res = get_args(arg, nextArg, global);
            if (res)
                return res;
            if (arg[1] != 'v')
            {
                i++;
            }
        }
        else
        {
            ReturnCode res = parse_url(arg, global);
            if (res)
                return res;
        }
    }
    ReturnCode res = add_default_headers(global);
    if (res)
        return res;
    return SUCCESS;
}

ReturnCode init_request(Global *global)
{
    Request *t_req = (Request *)malloc(sizeof(Request));
    if (t_req == NULL)
    {
        return ERR_MEMORY_ALLOCATION;
    }
    t_req->method = GET;
    t_req->url = NULL;
    t_req->headers = (HeaderList *)malloc(sizeof(HeaderList));
    if (t_req->headers == NULL)
    {
        return ERR_MEMORY_ALLOCATION;
    }
    t_req->headers->head = NULL;
    t_req->headers->tail = NULL;
    t_req->headers->size = 0;
    t_req->body = NULL;
    global->request = t_req;
    return SUCCESS;
}

ReturnCode get_args(char *flag, char *value, Global *global)
{
    switch (flag[1])
    {
    case 'm':
        if (strcmp(value, "GET") == 0)
        {
            global->request->method = GET;
        }
        else if (strcmp(value, "POST") == 0)
        {
            global->request->method = POST;
        }
        else if (strcmp(value, "PUT") == 0)
        {
            global->request->method = PUT;
        }
        else if (strcmp(value, "DELETE") == 0)
        {
            global->request->method = DELETE;
        }
        else
        {
            return ERR_INVALID_METHOD;
        }
        break;
    case 'h':
        char *key = strtok(value, ":");
        char *val = strtok(NULL, ":");
        if (key == NULL || val == NULL)
        {
            return ERR_INVALID_HEADER;
        }
        ReturnCode res = add_header(key, val, global, true);
        if (res)
        {
            return res;
        }
        break;
    case 'b':
        global->request->body = (char *)malloc(strlen(value) + 1);
        if (global->request->body == NULL)
        {
            return ERR_MEMORY_ALLOCATION;
        }
        strcpy(global->request->body, value);
        break;
    case 'v':
        global->is_verbose = true;
        break;
    default:
        return ERR_INVALID_ARGS;
    }
    return SUCCESS;
}

bool in_list(char *key, HeaderList *headers)
{
    Header *header = headers->head;
    for (int i = 0; i < headers->size; i++)
    {
        if (strcmp(key, header->key) == 0)
        {
            return true;
        }
        header = header->next;
    }
    return false;
}

ReturnCode add_default_headers(Global *global)
{
    if (!in_list("Host", global->request->headers))
    {
        ReturnCode res = add_header("Host", global->request->url->host, global, true);
        if (res)
        {
            return res;
        }
    }
    if (!in_list("Accept", global->request->headers))
    {
        ReturnCode res = add_header("Accept", "*/*", global, true);
        if (res)
        {
            return res;
        }
    }
    if (!in_list("Connection", global->request->headers))
    {
        ReturnCode res = add_header("Connection", "close", global, true);
        if (res)
        {
            return res;
        }
    }
    return SUCCESS;
}

ReturnCode add_header(char *key, char *value, Global *global, bool is_request)
{
    Header *header = malloc(sizeof(struct Header));
    if (header == NULL)
    {
        return ERR_MEMORY_ALLOCATION;
    }
    header->key = key;
    header->value = value;

    if (is_request)
    {
        if (global->request->headers->size == 0)
        {
            global->request->headers->head = header;
            global->request->headers->tail = header;
        }
        else
        {
            global->request->headers->tail->next = header;
            global->request->headers->tail = header;
        }
        global->request->headers->size++;
    }else{
        if (global->response->headers->size == 0)
        {
            global->response->headers->head = header;
            global->response->headers->tail = header;
        }
        else
        {
            global->response->headers->tail->next = header;
            global->response->headers->tail = header;
        }
        global->response->headers->size++;
    }
    return SUCCESS;
}

char *get_method(Method method)
{
    switch (method)
    {
    case GET:
        return "GET";
    case POST:
        return "POST";
    case DELETE:
        return "DELETE";
    case PUT:
        return "PUT";
    }
    return (char *)NULL;
}

char *request_to_string(const Request *req)
{
    char *request = malloc(1024);
    if (request == NULL)
    {
        return NULL;
    }
    snprintf(request, 1024, "%s %s HTTP/1.1\r\n", get_method(req->method), req->url->path);
    Header *header = req->headers->head;
    for (int i = 0; i < req->headers->size; i++)
    {
        strcat(request, header->key);
        strcat(request, ": ");
        strcat(request, header->value);
        strcat(request, "\r\n");
        header = header->next;
    }
    strcat(request, "\r\n");
    if (req->body != NULL)
    {
        strcat(request, req->body);
    }
    return request;
}

ReturnCode init_response(Global* global){
    Response* t_res = (Response*)malloc(sizeof(Response));
    if(t_res == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    t_res->version = NULL;
    t_res->status_code = 0;
    t_res->status_text = NULL;
    t_res->headers = (HeaderList*)malloc(sizeof(HeaderList));
    if(t_res == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    t_res->headers->head = NULL;
    t_res->headers->tail = NULL;
    t_res->headers->size = 0;
    t_res->body = NULL;
    global->response = t_res;
    return SUCCESS;
}

ReturnCode parse_response(char *response, Global *global)
{
    char* version = strtok(response, " ");
    if(version == NULL){
        return ERR_INVALID_RESPONSE;
    }
    global->response->version = (char*)malloc(strlen(version) + 1);
    if(global->response->version == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    strcpy(global->response->version, version);
    char* status_code = strtok(NULL, " ");
    if(status_code == NULL){
        return ERR_INVALID_RESPONSE;
    }
    global->response->status_code = atoi(status_code);
    char* status_text = strtok(NULL, "\r\n");
    if(status_text == NULL){
        return ERR_INVALID_RESPONSE;
    }
    global->response->status_text = (char*)malloc(strlen(status_text) + 1);
    if(global->response->status_text == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    strcpy(global->response->status_text, status_text);
    response += strlen(version) + strlen(status_code) + strlen(status_text) + 3 ;
    char* header_end = strstr(response, "\r\n\r\n");
    if(header_end == NULL){
        return ERR_INVALID_RESPONSE;
    }
    char* body = header_end + 4;
    global->response->body = (char*)malloc(strlen(body) + 1);
    if(global->response->body == NULL){
        return ERR_MEMORY_ALLOCATION;
    }
    strcpy(global->response->body, body);
    char* header = strtok(response, "\r\n");
    while(header != NULL){
        char* colon = strchr(header, ':');
        if(colon == NULL){
            return ERR_INVALID_RESPONSE;
        }
        *colon = '\0';
        char* key = header;
        colon++;
        while(*colon == ' '){
            colon++;
        }
        char* value = colon;
        if(key == NULL || value == NULL){
            return ERR_INVALID_RESPONSE;
        }
        ReturnCode res = add_header(key, value, global, false);
        if(res){
            return res;
        }
        header = strtok(NULL, "\r\n");
    }
    return SUCCESS;
}