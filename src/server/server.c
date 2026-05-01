#include "server.h"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

int8_t set_nonbocking(Socket_t  *sckt) {
	int flags = fcntl(sckt->socket_fd, F_GETFL, 0);
	if (flags == -1){
		char *msg = "Error - fcntl failed\n";
		write(1, msg, 22);
		return 1;
	}
	fcntl->socket_fd = fcntl(sckt_fd, F_SETFL, flags | 0_NONBLOCK); 
	return 0;
} 

int8_t create_server(Socket_t * sckt){
	sckt->socket_fd = socket(sckt->domain, sckt->type, sckt->protocol);
	if(sckt->socket_fd < 0){
		char *msg = "Error - failed to create socket\n";
		write(1, msg, 32);
		return 1;
	}
	if(bind(sckt->socket_fd, (struct sockaddr *) sckt->address, sizeof(sckt->address)) < 0) {
		char *msg = "Error - failed to bind socket\n";
        write(1, msg, 30);
        return 1;	

	}
	return 0;

}

int8_t start_listen(Socket_t * sckt){
	int *opt =1;
	setsockopt(sckt->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(listen(sckt->socket_fd, sckt->backlog) < 0){
		char *msg = "Error - failed to listen socket\n";
                write(1, msg, 32);
                return 1;
	
	}
	return 0;

} 

int8_t accept_new_connection(Connection_t *conn){
	struct scckaddr *client_addr;
	socklen_t client_len = sizeof(client_addr);
	conn->client_fd = accept(conn->server_fd, *client_addr, client_len); 
	if(conn->client_fd == -1){
		char *msg = "Error - accept returned -1\n";
		write(
		return 1;	
	}
	return 0;

}

int8_t read_client(Connection_t *conn){
	size_t total_read = 0;
	size_t bytes_read;

	while(1) {
		if (total_read + 4096 > conn->buffer_len){
			size_t new_size = conn->buffer_size *2;
			char *new_buffer = realloc(conn->buffer, new_size);
			if(!new_buffer){
				char *msg = "Error - realloc\n";
				write(1, msg, 17); 
				return -1;
			} 

			conn->buffer = new_buffer;
			conn->buffer_size = new_size;
			printf("New Buffer Size - %zu\n", new_size);
		}
		bytes_read = recv(conn->client_fd, conn->buffer + total_read, conn->buffer_len - total_read, 0);
		if(bytes_read <= 0){
			break;
		}
		total_read += bytes_read;
		
	}
	return 0;

}

int8_t read_buffer(Connection_t *conn){
	char *read_4_bytes = malloc(4);
	strcpy(read_4_bytes, conn->buffer);
	if(strstr(read_4_bytes, "http") != NULL){
		conn->res = init_http_response();
		conn->res = response_parser(response, conn->buffer);
		free(read_4_bytes);
		return 1;
	}else if{
		conn->req = init_http_request();
		conn->req = request_parser(request, conn->buffer);
		free(read_4_bytes);
		return 2;
	}else{
		free(read_4_bytes);
		return -1;
	}

	free(read_4_bytes);
	return 0;
	
}

char* get_ip_from_host(const char* hostname){
	struct addrinfo hints, *res;
	static char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostname, NULL, &hints, &res)  != 0) {
		return NULL;
	}

	void *addr;
	if (res->ai_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
		addr = &(ipv4->sin_addr);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
		addr = &(ipv6->sin6_addr);
	}

	inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
	freeaddrinfo(res);

	return ipstr;
	
}
