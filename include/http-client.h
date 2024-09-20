#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <netdb.h>
#include "utils.h"

ReturnCode get_addr_info(const Url* url, struct addrinfo** addr);

ReturnCode send_request(struct addrinfo* addr, const char* request, Global* global);

#endif  