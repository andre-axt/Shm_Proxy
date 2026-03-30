#include "server.h"
#include <sys/socket.h>
#include <unistd.h>

uint8_t create_server(Socket_t * sckt){
	sckt->socketfd = socket(sckt->domain, sckt->type, sckt->protocol);
	if(sckt->socket < 0){
		char *msg = "Error - failed to create socket");
		write(1, msg, 31);
		return 1;
	}
	if(bind(sckt->socketfd, (struct sockaddr *) sckt->sockaddr_in.address, sizeof(sckt->sockaddr_in.address)) < 0) {
		char *msg = "Error - failed to bind socket");
                write(1, msg, 29);
                return 1;	


	}
	return 0;

}
