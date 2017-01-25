#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "server.h"

// 构造response
#define RESPONSE                   \
    "HTTP/1.1 200 OK\r\n"          \
    "Content-Type: text/html\r\n"  \
    "Content-Length: 14\r\n"       \
    "\r\n"                         \
    "<h1>Hello</h1>"

uv_loop_t *loop;
uv_tcp_t server;
http_parser_settings settings;

struct sockaddr_in addr;

typedef struct {
    uv_tcp_t handle;
    http_parser parser;
    uv_buf_t resbuf;
    uv_write_t write_req;
} client_t;

void on_close(uv_handle_t *handle) {
    assert(handle->data == (client_t *)handle);
    free(handle->data);
}

void on_write(uv_write_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, ">>> on_write error: %s\n", uv_strerror(status));
        return;
    }
    uv_handle_t *handle = (uv_handle_t *)req->data;
    uv_close(handle, on_close);
}

int on_headers_complete(http_parser *parser) {
    client_t *client = (client_t *)(parser->data);
    (client->write_req).data = &(client->handle);
    uv_write(&(client->write_req), (uv_stream_t *)client, &client->resbuf, 1, on_write);
    return 1;
}

void on_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    client_t *client = ((uv_handle_t *)stream)->data;
    size_t parsed;
    if (nread >= 0) {
        /* parse http */
        parsed = http_parser_execute(&client->parser, &settings, buf->base, nread);
        if (parsed < nread) {
            fprintf(stderr, ">>> parsed error\n");
            uv_close((uv_handle_t *)stream, on_close);
        }
        
    } else {
        if (nread == UV_EOF) {
            uv_close((uv_handle_t *)stream, on_close);
        } else {
            fprintf(stderr, ">>> on_read error: %s\n", uv_strerror(nread));
        }
    }
    if (buf->base) {
        free(buf->base);
    }
}

void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char *)malloc(suggested_size), suggested_size);
}

void on_connection(uv_stream_t *server, int status) {
    int r;

    if (status < 0) {
        fprintf(stderr, ">>> on_connection error: %s\n", uv_strerror(status));
        return;
    }
    client_t *client = (client_t *)malloc(sizeof(client_t));
    client->handle.data = client; // restore
    client->parser.data = client; // restore
    uv_tcp_init(loop, &client->handle);
    r = uv_accept(server, (uv_stream_t *)&client->handle);
    if (r) {
        fprintf(stderr, ">>> uv_accept error: %s\n", uv_strerror(r));
        // close connection
        uv_close((uv_handle_t *)&client->handle, on_close);
    }
    // http_parser
    http_parser_init(&client->parser, HTTP_REQUEST);
    // response buffer
    (client->resbuf).base = RESPONSE;
    (client->resbuf).len = sizeof(RESPONSE);
    // start read
    uv_read_start((uv_stream_t *)&client->handle, on_alloc, on_read);
}

void run(char *host, int port) {
    int r;
    loop = uv_default_loop();

    settings.on_headers_complete = on_headers_complete;

    uv_tcp_init(loop, &server);
    uv_ip4_addr(host, port, &addr);
    r = uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    if (r) {
        fprintf(stderr, ">>> uv_tcp_bind error: %s\n", uv_strerror(r));
        exit(-1);
    }
    r = uv_listen((uv_stream_t*)&server, 120, on_connection);
    if (r) {
        fprintf(stderr, ">>> uv_listen error: %s\n", uv_strerror(r));
        exit(-1);
    }
    fprintf(stderr, ">> web server listen on <%s:%d>\n", host, port);

    uv_run(loop, UV_RUN_DEFAULT);
}

void http_server_init(http_server_t *server) {
    server->run = run;
}
