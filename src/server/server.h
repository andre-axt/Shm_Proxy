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
char* get_ip_from_host(const char* hostname);
ConnectionManager_t* init_connection_manager(int8_t max_conn, int epoll_fd);
int8_t add_client_connection(ConnectionManager_t *manager, int client_fd);
Connection_t* find_connection_by_fd(ConnectionManager *manager, int fd);
int8_t read_socket(Connection_t *conn, int8_t handler); // Set the handler to 1 to read the client socket and 2 to read the remote server socket, more features coming soon. :)
int8_t read_buffer(Connection_t *conn);

#endif
