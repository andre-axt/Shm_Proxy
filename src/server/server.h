#ifndef 
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>

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
	int	server_fd;
	char	buffer[BUFFER_SIZE];
	int	buffer_len;
	int	state;

} Connection_t;

int8_t create_server(Socket_t * sckt);
int8_t start_listen(Socket_t * sckt);
int8_t accept_client(Socket_t * server, Socket_t * client);
int8_t close_server(Socket_t * sckt);
int8_t set_nonblocking(int fd);

#endif
