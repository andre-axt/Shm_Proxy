#ifndef 
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10

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
	int	server_fd;
	char *buffer;
	int	buffer_len;
	int	state;

} Connection_t;

int8_t create_server(Socket_t * sckt);
int8_t start_listen(Socket_t * sckt);
int8_t close_server(Socket_t * sckt);
int8_t set_nonbocking(Socket_t  *sckt);
int8_t accept_new_connection(Connection_t *conn);
int8_t read_client(Connection_t *conn);
int8_t read_buffer(Connection_t *conn);

#endif
