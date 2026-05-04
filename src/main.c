#include "server.h"
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#define MAX_CONNECTIONS 100

Socket_t *server;
int epfd;
ConnectionManager_t *conn_manager;

void stop_server(int sig){
	if(server->socket_fd != -1 && server){
		close(server->socket_fd);
		free(server);
	} else if(server != NULL){
		free(server);
	}
	if(epfd != -1){
		close(epfd);
	}
	if(conn_manager != NULL){
		free_connection_manager(conn_manager);
	}
	exit(0);
}

int main(){
	signal(SIGINT, stop_server);
	server = malloc(sizeof(Socket_t));
	server->domain = AF_INET;
	server->type = SOCK_STREAM;
	server->protocol = 0;
	server->address.sin_family = AF_INET;
	server->address.sin_addr.s_addr = INADDR_ANY;
	server->address.sin_port = htons(PORT);

	epfd = epoll_create1(0);
	if(create_server(server)){
		return 1;

	}
	if(set_nonblocking(server->socket_fd)){
		return 1;
	
	}
	
    struct epoll_event ev, events[MAX_EVENTS]; 	
	if(start_listen(server)){
		return 1;
	}
	ev.events = EPOLLIN;
	ev.data.fd = server->socket_fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, server->socket_fd, &ev);
	conn_manager = init_connection_manager(MAX_CONNECTIONS, epfd);
	
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
				int fd = events[i].data.fd;
				Connection_t *conn = find_connection_by_fd(conn_manager, fd);

				if(conn == NULL) continue;

				if(fd == conn->client_fd && (events[i].events & EPOLLIN)){
					if(read_socket(conn, 1) == 0){
					
						if(conn->buffer){
							
							switch(read_buffer(conn)) {
								
								case 0:
									break;
								case -1:
									break;
								case 1:
									int result = send_buffer(conn, fd);
									if(result == 0){
										if("keep-alive" == get_header(conn->req->headers, conn->req->header_count, "Connection:")){
											conn->state = 0;
											conn->buffer_len = 0;
										} else {
											remove_connection(conn_manager, i);
										}
									} else if(result == 1) {
										struct epoll_event ev;
										ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
										ev.data.ptr = conn;
										epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev);
									} else {
										remove_connection(conn_manager, i);
									}
									break;
									
								case 2: 
									break;
								default:
									break;
							}
						}
					}
				}

				else if(fd == conn->remote_server_fd && (events[i].events & EPOLLIN)) {
					if(read_socket(conn, 2) == 0){
					
						if(conn->buffer){
							
							switch(read_buffer(conn)) {
								
								case 0:
									break;
								case -1:
									break;
								case 1:
									break;
									
								case 2: 
									const char* host = get_header(conn->req->headers, conn->req->header_count, "Host:");
									if(host){
										char *ip = get_ip_from_host(host);
										if(ip) {
											conn->remote_server_fd = socket(AF_INET, SOCK_STREAM, 0);

											if(conn->remote_server_fd < 0) {
												break;
											}

											struct sockaddr_in remote_addr;
											remote_addr.sin_family = AF_INET;
											remote_addr.sin_port = htons(80);
											if (inet_pton(remote_addr.sin_family, ip, &remote_addr.sin_addr) <= 0) {
												break;
											}
											
											if (connect(conn->remote_server_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0){
												if	(errno != EINPROGRESS) {
													remove_connection(conn_manager, i);
													break;
												}
												set_nonblocking(conn->remote_server_fd);
	
												struct epoll_event ev;
												ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
												ev.data.ptr = conn;
	
												epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev);
												send_buffer(conn, conn->remote_server_fd);
												free(ip);
												
											}
										} else {
											remove_connection(conn_manager, i);
										}
									}
									break;
								default:
									break;
							}
						}
					}
				}
				
			}
			
		
		}	
	
	}

	close(epfd);
	close(server->socket_fd);
	free(server);
	free_connection_manager(conn_manager);
}
