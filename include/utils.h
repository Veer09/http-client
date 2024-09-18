#ifndef UTILS_H
#define UTILS_H

typedef struct {
    char* protocol;
    char* host;
    char* port;
    char* path;
}Url;

typedef struct Header{
    char* key;
    char* value;
    struct Header* next;
}Header;

typedef struct {
    Header* head;
    Header* tail;
    size_t size;
}HeaderList;

typedef enum{
    GET = 0,
    POST, 
    PUT, 
    DELETE  
}Method;

typedef struct {
    Method method;
    Url* url;
    HeaderList* headers;
    char* body;
}Request;


typedef enum {
    SUCCESS = 0,                        
    ERR_INVALID_RESPONSE,      
    ERR_MEMORY_ALLOCATION,     
    ERR_UNKNOWN,
    ERR_INVALID_IPv6,
    ERR_INVALID_URL,
    ERR_INVALID_METHOD,
    ERR_INVALID_HEADER,
    ERR_INVALID_PROTOCOL,
    ERR_SYSTEM_ERROR,
    ERR_INVALID_ARGS              
}ReturnCode;

ReturnCode init_request(Request** req);

ReturnCode parse_args(int argc, char* argv[], Request* req);

ReturnCode parse_url(const char *url, Request* request);

ReturnCode add_header(char* key, char* value, Request* request);

ReturnCode add_default_headers(Request* request);

ReturnCode get_args(char* flag, char* value, Request* request);


void free_url(Url* url);


char* request_to_string(const Request* req);

void handle_error(ReturnCode code);

#endif