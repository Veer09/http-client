#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "../include/utils.h"
#include "../include/http-client.h"

ReturnCode get_addr_info(const Url *url, struct addrinfo **addr)
{
    int status;
    struct addrinfo hint;
    struct addrinfo *res;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_UNSPEC;     // For any type of address family
    hint.ai_socktype = SOCK_STREAM; // For TCP connection
    // DNS lookup
    if ((status = getaddrinfo(url->host, url->port, &hint, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return ERR_SYSTEM_ERROR;
    }
    *addr = res;
    printf("IP address %s\n", inet_ntoa(((struct sockaddr_in *)(res->ai_addr))->sin_addr));
    return SUCCESS;
}

ReturnCode send_request(struct addrinfo *addr, const char *request, Global *global)
{
    struct addrinfo *p;
    int socket_fd;
    for (p = addr; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Can't create socket!!");
            continue;
        }
        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(socket_fd);
            perror("Can't connect to server!!");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        perror("Can't connect to server!!");
        close(socket_fd);
        return ERR_SYSTEM_ERROR;
    }
    char *res = (char *)malloc(BUFSIZ);

    if (res == NULL)
    {
        close(socket_fd);
        return ERR_MEMORY_ALLOCATION;
    }

    if ((send(socket_fd, request, strlen(request), 0)) == -1)
    {
        close(socket_fd);
        perror("Can't send request!!");
        return ERR_SYSTEM_ERROR;
    }

    if (global->is_verbose)
    {

        Header *header = global->request->headers->head;

        for (int i = 0; i < global->request->headers->size; i++)
        {
            printf("> %s:%s\r\n", header->key, header->value);
            header = header->next;
        }
        printf("\r\n");
    }

    char *status_and_header = (char *)malloc(BUFSIZ);
    char buff[BUFSIZ];
    int recv_len = 0, total_len = 0;
    char *header_end;

    while ((recv_len = recv(socket_fd, buff, BUFSIZ - 1, 0)) > 0)
    {
        buff[recv_len] = '\0';
        if (total_len + recv_len > strlen(status_and_header))
        {
            status_and_header = (char *)realloc(status_and_header, total_len + recv_len + 1);
            if (status_and_header == NULL)
            {
                close(socket_fd);
                return ERR_MEMORY_ALLOCATION;
            }
        }
        memcpy(status_and_header + total_len, buff, recv_len + 1);
        total_len += recv_len;
        if ((header_end = strstr(status_and_header, "\r\n\r\n")) != NULL)
        {
            break;
        }
    }

    if (recv_len == -1)
    {
        perror("Can't receive response!!");
        close(socket_fd);
        return ERR_SYSTEM_ERROR;
    }

    if (header_end == NULL)
    {
        return ERR_INVALID_RESPONSE;
    }

    *header_end = '\0';

    char *body_part = (char *)malloc(strlen(header_end + 4));
    if (body_part == NULL)
    {
        close(socket_fd);
        return ERR_MEMORY_ALLOCATION;
    }
    strcpy(body_part, header_end + 4);

    init_response(global);
    parse_status_header(status_and_header, global);

    if (global->response->content_length > 0)
    {
        int remaining = global->response->content_length - strlen(body_part);
        total_len = strlen(body_part);
        while (remaining > 0)
        {
            recv_len = recv(socket_fd, buff, BUFSIZ - 1, 0);
            if (recv_len == -1)
            {
                perror("Can't receive response!!");
                close(socket_fd);
                return ERR_SYSTEM_ERROR;
            }
            buff[recv_len] = '\0';
            if (total_len + recv_len > strlen(body_part))
            {
                body_part = (char *)realloc(body_part, total_len + recv_len + 1);
                if (body_part == NULL)
                {
                    close(socket_fd);
                    return ERR_MEMORY_ALLOCATION;
                }
            }
            memcpy(body_part + total_len, buff, recv_len + 1);
            total_len += recv_len;
            remaining -= recv_len;

            char *end = strstr(body_part, "\r\n\r\n");
            if (end != NULL)
            {
                end += 4;
                *end = '\0';
                break;
            }
        }

        if (remaining < 0)
        {
            body_part[global->response->content_length] = '\0';
            printf("Warning: Content-Length is greater than actual content length provided in header\n");
        }
    }
    else if (global->response->is_chunked)
    {
        total_len = strlen(body_part);
        while ((recv_len = recv(socket_fd, buff, BUFSIZ - 1, 0)) > 0)
        {
            buff[recv_len] = '\0';
            if (total_len + recv_len > strlen(body_part))
            {
                body_part = (char *)realloc(body_part, total_len + recv_len + 1);
                if (body_part == NULL)
                {
                    close(socket_fd);
                    return ERR_MEMORY_ALLOCATION;
                }
            }
            memcpy(body_part + total_len, buff, recv_len + 1);
            total_len += recv_len;
        }
        char *response = (char *)malloc(1);
        response[0] = '\0';
        char *chunk_size_str_end = strstr(body_part, "\r\n");
        char *chunk_size_str = malloc(chunk_size_str_end - body_part + 1);
        if (chunk_size_str == NULL)
        {
            close(socket_fd);
            return ERR_MEMORY_ALLOCATION;
        }
        memcpy(chunk_size_str, body_part, chunk_size_str_end - body_part);
        chunk_size_str[chunk_size_str_end - body_part] = '\0';
        while (chunk_size_str != NULL)
        {
            int chunk_size = strtol(chunk_size_str, NULL, 16);
            if (chunk_size == 0)
            {
                break;
            }
            response = (char *)realloc(response, strlen(response) + chunk_size + 1);
            if (response == NULL)
            {
                close(socket_fd);
                return ERR_MEMORY_ALLOCATION;
            }
            memcpy(response + strlen(response), chunk_size_str_end + 2, chunk_size);
            body_part = chunk_size_str_end + 2 +  chunk_size + 2;
            response[strlen(response) + chunk_size] = '\0';
            chunk_size_str_end = strstr(body_part, "\r\n");
            chunk_size_str = malloc(chunk_size_str_end - body_part + 1);
            if (chunk_size_str == NULL)
            {
                close(socket_fd);
                return ERR_MEMORY_ALLOCATION;
            }
            memcpy(chunk_size_str, body_part, chunk_size_str_end - body_part);
            chunk_size_str[chunk_size_str_end - body_part] = '\0';
        }
        global->response->body = response;
    }
    else
    {
        char *end = strstr(body_part, "\r\n\r\n");
        if (end != NULL)
        {
            end += 4;
            *end = '\0';
        }
        else
        {
            total_len = strlen(body_part);
            while ((recv_len = recv(socket_fd, buff, BUFSIZ - 1, 0)) > 0)
            {
                buff[recv_len] = '\0';
                if (total_len + recv_len > strlen(body_part))
                {
                    body_part = (char *)realloc(body_part, total_len + recv_len + 1);
                    if (body_part == NULL)
                    {
                        close(socket_fd);
                        return ERR_MEMORY_ALLOCATION;
                    }
                }
                memcpy(body_part + total_len, buff, recv_len + 1);
                total_len += recv_len;

                if ((end = strstr(body_part, "\r\n\r\n")) != NULL)
                {
                    end += 4;
                    *end = '\0';
                    break;
                }
            }
        }
        global->response->body = body_part;
    }

    if (global->is_verbose)
    {
        printf("< %s %d %s\r\n", global->response->version, global->response->status_code, global->response->status_text);
        Header *header = global->response->headers->head;
        for (int i = 0; i < global->response->headers->size; i++)
        {
            printf("< %s:%s\r\n", header->key, header->value);
            header = header->next;
        }
        printf("\r\n");
    }

    printf("%s", global->response->body);
    return SUCCESS;
}
