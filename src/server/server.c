#include "server.h"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>

int8_t set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1){
		char *msg = "Error - fcntl failed\n";
		write(1, msg, 22);
		return 1;
	}
	fd = fcntl(fd, F_SETFL, flags | O_NONBLOCK); 
	return 0;
} 

int8_t create_server(Socket_t * sckt){
	sckt->socket_fd = socket(sckt->domain, sckt->type, sckt->protocol);
	if(sckt->socket_fd < 0){
		char *msg = "Error - failed to create socket\n";
		write(1, msg, 32);
		return -1;
	}
	if(bind(sckt->socket_fd, (struct sockaddr *) &sckt->address, sizeof(sckt->address)) < 0) {
		char *msg = "Error - failed to bind socket\n";
        write(1, msg, 30);
        return 1;	

	}
	return 0;

}

int8_t start_listen(Socket_t * sckt){
	int opt = 1;
	setsockopt(sckt->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(listen(sckt->socket_fd, sckt->backlog) < 0){
		char *msg = "Error - failed to listen socket\n";
                write(1, msg, 32);
                return 1;
	
	}
	return 0;

} 

int8_t read_socket(Connection_t *conn, int8_t handler){
	size_t total_read = 0;
	size_t bytes_read;
	if(handler > 2 || handler < 1){
		char *msg = "Error - handler must be 1 or 2\n";
		write(1, msg, 32);
		return -1;
	}
	while(1) {
		if (total_read + 4096 > conn->buffer_len){
			size_t new_size = conn->buffer_len *2;
			char *new_buffer = realloc(conn->buffer, new_size);
			if(!new_buffer){
				char *msg = "Error - realloc\n";
				write(1, msg, 17); 
				return -1;
			} 

			conn->buffer = new_buffer;
			conn->buffer_len = new_size;
			char msg[50];

			int8_t msg_size = snprintf(msg, sizeof(msg), "New Buffer Size: %zu\n", new_size);
			write(1, msg, msg_size);
			printf("New Buffer Size - %zu\n", new_size);
		}
		if(handler == 1) {
			bytes_read = recv(conn->client_fd, conn->buffer + total_read, conn->buffer_len - total_read, 0);
			if(bytes_read <= 0){
				break;
			}
			total_read += bytes_read;
		}else {
			bytes_read = recv(conn->remote_server_fd, conn->buffer + total_read, conn->buffer_len - total_read, 0);
			if(bytes_read <= 0){
				break;
			}
			total_read += bytes_read;
		}
		
	}
	return 0;

}

int8_t read_buffer(Connection_t *conn) {
    const char *methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"};
    int8_t num_methods = sizeof(methods) / sizeof(methods[0]);
	
    if (strncmp(conn->buffer, "HTTP", 4) == 0) {
        conn->res = init_http_response();
        conn->res = response_parser(conn->res, conn->buffer);
        return 1;
    }

    for (int i = 0; i < num_methods; i++) {
        if (strncmp(conn->buffer, methods[i], strlen(methods[i])) == 0) {
			size_t len = strlen(methods[i]);
			if (conn->buffer[len] == ' ' || conn->buffer[len] == '\0') {
	            conn->req = init_http_request();
	            conn->req = request_parser(conn->req, conn->buffer);
				return 2; 
		    }
        }
    }

    return -1; 
}

char* get_ip_from_host(const char* hostname){
	struct addrinfo hints, *results;
	static char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostname, NULL, &hints, &results)  != 0) {
		return NULL;
	}

	void *addr;
	if (results->ai_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)results->ai_addr;
		addr = &(ipv4->sin_addr);
	} else {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)results->ai_addr;
		addr = &(ipv6->sin6_addr);
	}

	inet_ntop(results->ai_family, addr, ipstr, sizeof(ipstr));
	freeaddrinfo(results);

	return ipstr;
	
}

ConnectionManager_t* init_connection_manager(uint8_t max_conn, int epoll_fd) {
	ConnectionManager_t *manager = malloc(sizeof(ConnectionManager_t));
	manager->conn = calloc(max_conn, sizeof(Connection_t));
	manager->max_conn = max_conn;
	manager->act_conn = 0;
	manager->epoll_fd = epoll_fd;

	for(int i = 0; i < max_conn; i++) {
		manager->conn[i].client_fd = -1;
		manager->conn[i].remote_server_fd = -1;
		manager->conn[i].buffer = malloc(BUFFER_SIZE);
		manager->conn[i].buffer_len = 0;
		manager->conn[i].state = 0;
		manager->conn[i].res = malloc(sizeof(http_response_t));
		manager->conn[i].req = malloc(sizeof(http_request_t));
	}

	return manager;
}

void free_connection_manager(ConnectionManager_t* conn_manager){
	if (!conn_manager) return;

	for (int i = 0; i < conn_manager->act_conn; i++){
		if (conn_manager->conn[i].buffer) {
			free(conn_manager->conn[i].buffer);
		
		}
		if (conn_manager->conn[i].res) {
			free(conn_manager->conn[i].res);
		
		}
		if (conn_manager->conn[i].req) {
			free(conn_manager->conn[i].req);

		}
	
	}

	free(conn_manager->conn);
	free(conn_manager);

}

Connection_t* add_client_connection(ConnectionManager_t *manager, int client_fd) {
	for(int i = 0; i < manager->max_conn; i++) {
		if(manager->conn[i].client_fd == -1) {
			manager->conn[i].client_fd = client_fd;
			manager->conn[i].state = 0;
			manager->conn[i].buffer_len = 0;
			manager->act_conn++;

			struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.ptr = &manager->conn[i];

			epoll_ctl(manager->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

			return &manager->conn[i];
		}
	}
	return NULL;
}

Connection_t * find_connection_by_fd(ConnectionManager_t *manager, int fd) {
	for(int i = 0; i < manager->max_conn; i++) {
		if(manager->conn[i].client_fd == fd || manager->conn[i].remote_server_fd == fd){
			return &manager->conn[i]; //It simply return the connection, without indicating whether it's the client's or the remote_server's fd  
		}
	}
	return NULL;
}

int find_idx_by_fd(ConnectionManager_t *manager, int fd) {
	for(int i = 0; i < manager->max_conn; i++) {
		if(manager->conn[i].client_fd == fd || manager->conn[i].remote_server_fd == fd){
			return i;
		}
	}
	return -1;
}

int8_t send_buffer(Connection_t *conn, int fd) {
	if(fd == -1) return -1;

	ssize_t sent = send(fd, conn->buffer, conn->buffer_len, 0);
	if(sent < 0) {
		if(errno == EAGAIN || errno == EWOULDBLOCK) {
			return 1;
		}
		return -1;
	}
	if((size_t)sent < conn->buffer_len) {
		memmove(conn->buffer, conn->buffer + sent, conn->buffer_len - sent);
		conn->buffer_len -= sent;
		return 1;
	}

	conn->buffer_len = 0;
	return 0;
}

void remove_connection(ConnectionManager_t *manager, int index) {
	if(index < 0 || index >= manager->max_conn) return;
	Connection_t *conn = &manager->conn[index];

	if(conn->client_fd != -1) {
		epoll_ctl(manager->epoll_fd, EPOLL_CTL_DEL, conn->client_fd, NULL);
		close(conn->client_fd);
		conn->client_fd = -1;
	}
	if(conn->remote_server_fd != -1) {
		epoll_ctl(manager->epoll_fd, EPOLL_CTL_DEL, conn->remote_server_fd, NULL);
		close(conn->remote_server_fd);
		conn->remote_server_fd = -1;
	}
	
	conn->buffer_len = 0;
	conn->state = 0;
	manager->act_conn--;
}
