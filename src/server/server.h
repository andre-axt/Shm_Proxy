#ifndef 
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/epoll.h>
#include "http_parser.h"

#define MAX_EVENTS 10
#define BUFFER_SIZE 8192

typedef struct {
	int	socket_fd;
	struct 	sockaddr_in address;
	int 	port;
	int	backlog;
	int	domain;
	int	type;
	int	protocol;
} Socket_t;

typedef struct {
	int	client_fd;
	int	remote_server_fd; 
	char *buffer;
	size_t buffer_len;
	int	state;
	http_response_t *res;
	http_request_t *req;
} Connection_t;

typedef struct {
	Connection_t *connections;
	int8_t max_conn;
	int8_t act_conn;
	int epoll_fd;
} ConnectionManager_t;

int8_t create_server(Socket_t * sckt);
int8_t start_listen(Socket_t * sckt);
int8_t close_server(Socket_t * sckt);
int8_t set_nonbocking(Socket_t  *sckt);
int8_t accept_new_connection(Connection_t *conn, int *server_fd);
char* get_ip_from_host(const char* hostname);
int8_t read_client(Connection_t *conn);
int8_t read_buffer(Connection_t *conn);

#endif
