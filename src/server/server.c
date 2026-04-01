#include "server.h"
#include <sys/socket.h>
#include <unistd.h>

int8_t create_server(Socket_t * sckt){
	sckt->socket_fd = socket(sckt->domain, sckt->type, sckt->protocol);
	if(sckt->socket_fd < 0){
		char *msg = "Error - failed to create socket";
		write(1, msg, 31);
		return 1;
	}
	if(bind(sckt->socket_fd, (struct sockaddr *) sckt->sockaddr_in.address, sizeof(sckt->sockaddr_in.address)) < 0) {
		char *msg = "Error - failed to bind socket";
                write(1, msg, 29);
                return 1;	

	}
	return 0;

}

int8_t	start_listen(Socket_t * sckt){
	int *opt =1;
	setsockopt(sckt->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(listen(sckt->socket_fd, sckt->backlog) < 0){
		char *msg = "Error - failed to listen socket";
                write(1, msg, 31);
                return 1;
	
	}
	return 0;

} 

