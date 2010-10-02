#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ev.h>
#include <website.h>

int connections = 0;
int requests = 0;
int current_connections = 0;
int current_requests = 0;
int host_header = -1;
int realip_header = -1;

char testdata0[] =
    "<!DOCTYPE html>\n";
char testdata1[] =
    "<html>\n"
    "  <head><title>Hello from sample</title></head>\n";
char testdata2[] =
    "  <body>\n"
    "    <h1>Hello from sample</h1>\n"
    "  </body>\n"
    "</html>\n";

typedef struct myconnection_s {
    ws_connection_t native;
    int id;
} myconnection_t;

typedef struct myrequest_s {
    ws_request_t native;
    int id;
} myrequest_t;

int incr_connections(myconnection_t *conn) {
    printf("incr_connections\n");
    conn->id = connections++;
    ++current_connections;
}

int decr_connections(myconnection_t *conn) {
    printf("decr_connections\n");
    --current_connections;
}

int incr_requests(myrequest_t *req) {
    printf("incr_requests\n");
    req->id = requests++;
    ++current_requests;
}

int decr_requests(myrequest_t *req) {
    printf("decr_requests\n");
    --current_requests;
}

int reply(myrequest_t *req) {
    char buf[32];
    printf("replying...\n");
    ws_statusline(&req->native, "200 OK");
    sprintf(buf, "%d", req->id);
    ws_add_header(&req->native, "X-Request-ID", buf);
    sprintf(buf, "%d", ((myconnection_t*)req->native.conn)->id);
    ws_add_header(&req->native, "X-Connection-ID", buf);
    ws_add_header(&req->native, "Content-Type", "text/html");
    ws_finish_headers(&req->native);
    obstack_blank(&req->native.pieces, 0);
    obstack_grow(&req->native.pieces, testdata0, sizeof(testdata0)-1);
    obstack_grow(&req->native.pieces, testdata1, sizeof(testdata1)-1);
    obstack_grow(&req->native.pieces, testdata2, sizeof(testdata2)-1);
    int len = obstack_object_size(&req->native.pieces);
    ws_reply_data(&req->native, obstack_finish(&req->native.pieces), len);
    return 0;
}


int main(int argc, char **argv) {
    struct ev_loop *loop = ev_default_loop(0);
    ws_server_t server;
    ws_server_init(&server, loop);
    if(ws_add_tcp(&server, inet_addr("127.0.0.1"), 80) < 0) {
        perror("Error binding port 80");
    }
    ws_add_tcp(&server, inet_addr("127.0.0.1"), 8080);
    ws_add_unix(&server, "/tmp/ws_test_sock", strlen("/tmp/ws_test_sock"));
    //ws_add_fd(&server, 0);
    ws_CONNECTION_STRUCT(&server, myconnection_t);
    ws_REQUEST_STRUCT(&server, myrequest_t);
    ws_CONNECT_CB(&server, incr_connections);
    ws_DISCONNECT_CB(&server, decr_connections);
    ws_HEADERS_CB(&server, incr_requests);
    ws_REQUEST_CB(&server, reply);
    ws_FINISH_CB(&server, decr_requests);
    host_header = ws_index_header(&server, "Host");
    realip_header = ws_index_header(&server, "X-Real-IP");
    ws_server_start(&server);
	ev_loop (loop, 0);
	return 0;
}
