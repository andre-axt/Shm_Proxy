#include "server.h"
#include "shm_cache.h"

int main(){
	Socket_t *server;
	Connection_t * conn;
	server->domain = AF_INET;
	server->type = SOCK_STREAM;
	server->protocol = 0;
	server->address.sin_family = AF_INET;
	server->address.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);

	Cache_t *cache;
	int epfd = epoll_create1(0);
	if(create_server(server)){
		return 1;

	}
	init_cache(cache)
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
	Connection_t * conn = malloc(sizeof(Connection_t);
	conn->buffer = malloc(BUFFER_SIZE);
	conn->buffer_len = (size_t) BUFFER_SIZE;
	while(1){
		int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
	       	
		for (int i = 0; i <= MAX_EVENTS; i++){
			
			if(events[i].data.fd = server->socket_fd){
				accept_new_connection(conn, server->socket_fd);

			}
			else if(events[i].data.fd > 0){
				conn->client_fd = events[i].data.fd;
				if(read_client(conn) = 0){
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
