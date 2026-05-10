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
	switch(create_server(server)){
		case -1: {
			free(server);
			char *msg = "Error - socket create failed\n";
			write(1, msg, 29);
			return 1;
		}
			break;
		case 1: {
			free(server);
			close(server->socket_fd);
			char *msg = "Error - bind socket failed\n";
			write(1, msg, 27);
			return 1;
		}
			break;
	}
	if(set_nonblocking(server->socket_fd)){
		close(server->socket_fd);
		free(server);
		char *msg = "Error - set nonblocking failed\n";
		write(1, msg, 31);
		return 1;
	
	}
	
    struct epoll_event ev, events[MAX_EVENTS]; 	
	if(start_listen(server)){
		close(server->socket_fd);
		free(server);
		char *msg = "Error - listen socket failed\n";
		write(1, msg, 29);
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
					Connection_t *conn = add_client_connection(conn_manager, client_fd);
					if(conn == NULL) {
						char *msg = "Error - add client to epoll failed\n";
						write(1, msg, 35);
						close(client_fd);
					}
					conn->remote_server_fd = socket(AF_INET, SOCK_STREAM, 0);
			        if(conn->remote_server_fd < 0) {
						int idx = find_idx_by_fd(conn_manager, conn->remote_server_fd);
						if(idx == -1) break;			
						remove_connection(conn_manager, idx);
			            continue;
			        }
				}

			}
			else if(events[i].data.fd > 0){
				int fd = events[i].data.fd;
				Connection_t *conn = find_connection_by_fd(conn_manager, fd);

				if(conn == NULL){
					char *msg = "Error - find connection by fd failed\n";
					write(1, msg, 36);
					continue;	
				} 

				if(fd == conn->client_fd && (events[i].events & EPOLLIN)){
					if(read_socket(conn, 1) == 0){
					
						if(conn->buffer){
							
							switch(read_buffer(conn)) {
								case -1: {
									char *msg = "Error - read buffer failed\n";
									write(1, msg, 28);
								}
									break;
								case 1: {
									const char* host = get_header(conn->req->headers, conn->req->header_count, "Host:");
									if(host){
										char *ip = get_ip_from_host(host);
										if(ip) {
											Socket_t *remote_server = malloc(sizeof(Socket_t));
											remote_server->address.sin_addr.s_addr = inet_addr(ip);
											remote_server->address.sin_port = htons(80);

											if(connect(conn->remote_server_fd, (struct sockaddr*)&remote_server->address, sizeof(remote_server->address)) < 0){
												if(errno != EINPROGRESS) {
													int idx = find_idx_by_fd(conn_manager, fd);
													if(idx == -1) break;
													remove_connection(conn_manager, idx);
													free(remote_server);
													break;
												}
												conn->state = 2; 
											}

											struct epoll_event ev;
											ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
											ev.data.ptr = conn;
											epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev);

											conn->remote_server_fd = remote_server->socket_fd;
										}
									}
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

				else if(fd == conn->remote_server_fd && (events[i].events & EPOLLOUT)) {
					if(conn->state == 2) {
						conn->state = 3;

						if(conn->req != NULL) {
							int8_t sent = send_buffer(conn, conn->remote_server_fd);
							while(sent == 1){
								sent = send_buffer(conn, conn->remote_server_fd);
							}
						}

						struct epoll_event ev;
						ev.events = EPOLLIN | EPOLLET;
						ev.data.ptr = conn;
						epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev);
					}
				}
					
				else if(fd == conn->remote_server_fd && (events[i].events & EPOLLIN)) {
					if(read_socket(conn, 2) == 0){
					
						if(conn->buffer){
							
							read_buffer(conn);
							int8_t sent = send_buffer(conn, conn->client_fd);
							if(sent == -1){
								int idx = find_idx_by_fd(conn_manager, conn->client_fd);
								if(idx < 0) break;
								remove_connection(conn_manager, idx);
								break;	
							} 
							while(sent == 1){
								send_buffer(conn, conn->client_fd);
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
