#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "http_parser.h"

#define MAX_EVENTS 100
#define BUFFER_SIZE 8192
#define PORT 8080

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
	char	*buffer;
	size_t 	buffer_len;
	int8_t	state; // 1 = STATE_WAITING_HOST, 2 = STATE_CONNECTING, 3 = STATE_CONNECTED, 4 = STATE_CLOSED
	http_response_t *res;
	http_request_t *req;
} Connection_t;

typedef struct {
	Connection_t *conn;
	uint8_t max_conn;
	uint8_t act_conn;
	int epoll_fd;
} ConnectionManager_t;

int8_t create_server(Socket_t * sckt);
int8_t start_listen(Socket_t * sckt);
int8_t close_server(Socket_t * sckt);
int8_t set_nonblocking(int fd);
char* get_ip_from_host(const char* hostname);
ConnectionManager_t* init_connection_manager(uint8_t max_conn, int epoll_fd);
void free_connection_manager(ConnectionManager_t* conn_manager);
Connection_t* add_client_connection(ConnectionManager_t *manager, int client_fd);
Connection_t* find_connection_by_fd(ConnectionManager_t *manager, int fd);
int find_idx_by_fd(ConnectionManager_t *manager, int fd);
int8_t send_buffer(Connection_t *conn, int fd);
void remove_connection(ConnectionManager_t *manager, int index);
int8_t read_socket(Connection_t *conn, int8_t handler); // Set the handler to 1 to read the client socket and 2 to read the remote server socket, more features coming soon. :)
int8_t read_buffer(Connection_t *conn);

#endif
