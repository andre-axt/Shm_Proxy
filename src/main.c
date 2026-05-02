#include "server.h"
#include "shm_cache.h"

#define MAX_CONNECTIONS 1000

int main(){
	Socket_t *server = malloc(sizeof(Socket_t));
	server->domain = AF_INET;
	server->type = SOCK_STREAM;
	server->protocol = 0;
	server->address.sin_family = AF_INET;
	server->address.sin_addr.s_addr = INADDR_ANY;
	server->address.sin_port = htons(PORT);

	Cache_t *cache = malloc(sizeof(Cache_t));
	int epfd = epoll_create1(0);
	if(create_server(server)){
		return 1;

	}
	init_cache(cache);
	if(set_nonblocking(server)){
		return 1;
	
	}
	
    struct epoll_event ev, events[MAX_EVENTS]; 	
	if(start_listen(server)){
		return 1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = server->socket_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server->socket_fd, &ev);
	ConnectionManager_t *conn_manager = init_connection_manager(MAX_CONNECTIONS, epfd);
	
	while(1){
		int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
	       	
		for (int i = 0; i < nfds; i++){
			
			if(events[i].data.fd == server->socket_fd){
				struct sockaddr_in client_addr;
				socklen_t addr_len = sizeof(client_addr);
				int client_fd = accept(server->socket_fd, (struct sockaddr*)&client_addr, &addr_len);

				if(client_fd > 0) {
					set_nonblocking(client_fd);
					int8_t idx = add_client_connection(conn_manager, client_fd);
					if(idx == -1) {
						close(client_fd);
					}
				}

			}
			else if(events[i].data.fd > 0){
				conn->client_fd = events[i].data.fd;
				if(read_client(conn) == 0){
					if(conn->buffer > 0){
						switch(read_buffer(conn)) {
							case 0:
								break;
							case -1:
								break;
							case 2:
								const char* host = get_headers(conn->res->headers, conn->res->header_count, "Host:");
								if(host){
									char *ip = get_ip_from_host(host);
									if(ip) {
										co
									}
								}
						}
						
					}	
				}
			}
			
		
		}	
	
	}
	free(conn->buffer);
	free(conn);
}
