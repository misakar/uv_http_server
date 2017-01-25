# uv_http_server

a simple http server on top of [libuv](https://github.com/libuv/libuv)❤️ [http-parser](https://github.com/nodejs/http-parser)

## how 2 use
**main.c**

    #include "server.h"
    #include <stdio.h>
    #include <stdlib.h>
    
    #define HOST "0.0.0.0"
    #define PORT 4399
    
    void main(int argc, char **argv) {
        http_server_t *server = malloc(sizeof(http_server_t));
    
        http_server_init(server);
    
        server->run(HOST, PORT);
    
        free(server);
    } 

**Makefile**

    uv = /path/to/libuv.a
    http_parser = /path/to/libhttp_parser.a
    pthread = -lpthread
    cc = gcc

    test_server: main.c server.c
	    $(cc) -o test_server main.c server.c $(uv) $(http_parser) $(pthread)
