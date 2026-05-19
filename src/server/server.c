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
	ssize_t bytes_read;
	if(handler > 2 || handler < 1){
		char *msg = "Error - handler must be 1 or 2\n";
		write(1, msg, 32);
		return -1;
	}
	if(handler == 1){
		size_t total_read = conn->client_buffer_len;
		while(1) {
			if (total_read + 4096 > conn->client_buffer_cap){
				size_t new_size = conn->client_buffer_cap * 2;
				if(new_size == 0) new_size = BUFFER_SIZE;
				
				char *new_buffer = realloc(conn->client_buffer, new_size);
				if(!new_buffer){
					char *msg = "Error - realloc\n";
					write(1, msg, 17); 
					return -1;
				} 
	
				conn->client_buffer = new_buffer;
				conn->client_buffer[conn->client_buffer_len + 1] = '\0';
				conn->client_buffer_cap = new_size;
			}
			
			bytes_read = recv(conn->client_fd, conn->client_buffer + total_read, conn->client_buffer_cap - total_read, 0);
				
			if (bytes_read > 0){
				total_read += bytes_read;
				continue;
			} else if (bytes_read == 0) {
				break;
			}
			else {
				if(errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				}
	
				return -1;
			}
			
		}

		conn->client_buffer_len = total_read;
		
		if (total_read >= conn->client_buffer_cap) {
			
	        conn->client_buffer_cap = total_read + 1;
	        conn->client_buffer = realloc(conn->client_buffer, conn->client_buffer_cap);
	    }
	    conn->client_buffer[total_read] = '\0';
		return 0;
	}
	if(handler == 2){
		size_t total_read = conn->remote_server_buffer_len;
		while(1) {
			if (total_read + 4096 > conn->remote_server_buffer_cap){
				size_t new_size = conn->remote_server_buffer_cap * 2;
				if(new_size == 0) new_size = BUFFER_SIZE;
				
				char *new_buffer = realloc(conn->remote_server_buffer, new_size);
				if(!new_buffer){
					char *msg = "Error - realloc\n";
					write(1, msg, 17); 
					return -1;
				} 
	
				conn->remote_server_buffer = new_buffer;
				conn->remote_server_buffer[conn->remote_server_buffer_len + 1] = '\0';
				conn->remote_server_buffer_cap = new_size;
			}
			
			bytes_read = recv(conn->remote_server_fd, conn->remote_server_buffer + total_read, conn->remote_server_buffer_cap - total_read, 0);
				
			if (bytes_read > 0){
				total_read += bytes_read;
				continue;
			} else if (bytes_read == 0) {
				break;
			}
			else {
				if(errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				}
	
				return -1;
			}
			
		}
	
		conn->remote_server_buffer_len = total_read;
	
		if(total_read >= conn->remote_server_buffer_cap) {
			conn->remote_server_buffer_cap = total_read + 1;
			conn->remote_server_buffer = realloc(conn->remote_server_buffer, conn->remote_server_buffer_cap);
		}
		conn->remote_server_buffer[total_read] = '\0';
		return 0;
	}
	

}

int8_t read_buffer(Connection_t *conn, int8_t handler) {
    const char *methods[] = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS", "CONNECT"};
    int8_t num_methods = sizeof(methods) / sizeof(methods[0]);
	if(handler > 2 || handler < 1) return -1;
	
	if(handler == 1){
	    if (strncmp(conn->remote_server_buffer, "HTTP", 4) == 0) {
	        conn->res = response_parser(conn->res, conn->remote_server_buffer);
	        return 0;
	    }
		
	}
	else{
	    for (int i = 0; i < num_methods; i++) {
	        if (strncmp(conn->client_buffer, methods[i], strlen(methods[i])) == 0) {
				size_t len = strlen(methods[i]);
				if (conn->client_buffer[len] == ' ' || conn->client_buffer[len] == '\0') {
		            conn->req = request_parser(conn->req, conn->client_buffer);
					return 0; 
			    }
	        }
	    }
		
	}

    return 1; 
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
		manager->conn[i].client_buffer = malloc(BUFFER_SIZE);
		manager->conn[i].remote_server_buffer = malloc(BUFFER_SIZE);
		manager->conn[i].client_buffer_len = 0;
		manager->conn[i].remote_server_buffer_len = 0;
		manager->conn[i].client_buffer_cap = BUFFER_SIZE;
		manager->conn[i].remote_server_buffer_cap = BUFFER_SIZE;
		manager->conn[i].state = 0;
		manager->conn[i].res = NULL;
		manager->conn[i].req = NULL;
	}

	return manager;
}

void free_connection_manager(ConnectionManager_t* conn_manager){
	if (!conn_manager) return;

	for (int i = 0; i < conn_manager->max_conn; i++){
		if (conn_manager->conn[i].client_buffer) {
			free(conn_manager->conn[i].client_buffer);
		}
		if (conn_manager->conn[i].remote_server_buffer) {
			free(conn_manager->conn[i].remote_server_buffer);
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
			manager->conn[i].client_buffer_len = 0;
			manager->conn[i].remote_server_buffer_len = 0;
			manager->act_conn++;

			struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = client_fd;

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
    if(fd == -1) return 0;

    char *buf_ptr = NULL;
    size_t *buf_len = NULL;
	
    if (fd == conn->remote_server_fd) {
        buf_ptr = conn->client_buffer;
        buf_len = &conn->client_buffer_len;
		
    } else if (fd == conn->client_fd) {
        buf_ptr = conn->remote_server_buffer;
        buf_len = &conn->remote_server_buffer_len;
		
    }

    if (buf_ptr == NULL || *buf_len == 0) return 0;

    ssize_t sent = send(fd, buf_ptr, *buf_len, 0);
    if(sent < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1; 
        }
        return -1; 
    }
    else if(sent == 0){
        return -1;
    }
    else if((size_t)sent < *buf_len) {
        memmove(buf_ptr, buf_ptr + sent, *buf_len - sent);
        *buf_len -= sent;
        return 1; 
    }

    *buf_len = 0; 
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
	
	conn->client_buffer_len = 0;
	conn->remote_server_buffer_len = 0;
	conn->state = 0;
	manager->act_conn--;
}
