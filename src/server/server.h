#ifndef 
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>

typedef struct {
	int	socketfd;
	struct 	sockaddr_in address;
	int 	port;
	int	backlog;
	int	domain;
	int	type;
	int	protocol;
} Socket_t;

uint8_t create_server(Socket_t * sckt);
uint8_t start_listen(Socket_t * sckt);
uint8_t accept_client(Socket_t * server, Socket_t * client);
uint8_t close_server(Socket_t * sckt);
