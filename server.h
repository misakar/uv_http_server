#ifndef SERVER_H
#define SERVER_H

#include <uv.h>
#include <http_parser.h>

struct http_server_s {
    void (*run)(char*, int);
};

typedef struct http_server_s http_server_t;

void http_server_init(http_server_t*);

#endif
