#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <netdb.h>
#include "utils.h"

void get_addr_info(const struct url_info* url, struct addrinfo** addr);

void send_request(struct addrinfo* addr, const char* request);

#endif  