#ifndef UTILS_H
#define UTILS_H

struct url_info {
    char *protocol;
    char *host;
    char *port;
    char *path;
}; 

struct request{
    char* method;
    char* path;
    char* host;
    int port;
    char* header;
};

void parse_url(const char *url, struct url_info** p_url);

void free_url(struct url_info* url);

struct request* create_request(const char* method, const char* header, const struct url_info* url);

char* request_to_string(const struct request* req);

#endif