#include "server.h"
#include "shm_cache.h"

int main(){
	Socket_t *server;
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
	while(1){
		int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
	       		
	
	}
}
