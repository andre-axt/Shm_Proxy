#include "shm_cache.h"
#include "server.h"
#include "config.h"
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define MAX_CONNECTIONS 100

Socket_t *server;
int epfd;
ConnectionManager_t *conn_manager;
Cache_t* cache;

void stop_server(int sig){
    if(server && server->socket_fd != -1){
        close(server->socket_fd);
        free(server);
		server = NULL;
    } else if(server != NULL){
        free(server);
		server = NULL;
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
    signal(SIGPIPE, SIG_IGN);	
    signal(SIGINT, stop_server);
    server = malloc(sizeof(Socket_t));
	if (server == NULL) return 1;
    server->domain = AF_INET;
    server->type = SOCK_STREAM;
    server->protocol = 0;
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(BIND_PORT);
    server->backlog = MAX_CONNECTIONS;  
	
    epfd = epoll_create1(0);
    
    switch(create_server(server)){
        case -1: {
            free(server);
			server = NULL;
            char *msg = "Error - socket create failed\n";
            write(1, msg, 29);
            return 1;
        }
        case 1: {
            close(server->socket_fd);
            free(server);
			server = NULL;
            char *msg = "Error - bind socket failed\n";
            write(1, msg, 27);
            return 1;
        }
    }
    
    if(set_nonblocking(server->socket_fd)){
        close(server->socket_fd);
        free(server);
		server = NULL;
        char *msg = "Error - set nonblocking failed\n";
        write(1, msg, 31);
        return 1;
    }
    
    struct epoll_event ev, events[MAX_EVENTS];    
    if(start_listen(server)){
        close(server->socket_fd);
        free(server);
		server = NULL;
        char *msg = "Error - listen socket failed\n";
        write(1, msg, 29);
        return 1;
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = server->socket_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server->socket_fd, &ev);
    
    conn_manager = init_connection_manager(MAX_CONNECTIONS, epfd);
	cache = init_cache();
    
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
                        continue; 
                    }
                    
                    conn->remote_server_fd = socket(AF_INET, SOCK_STREAM, 0);
                    if(conn->remote_server_fd < 0) {
                        int idx = find_idx_by_fd(conn_manager, client_fd);
                        if(idx != -1) remove_connection(conn_manager, idx);
                        continue;
                    }
                    set_nonblocking(conn->remote_server_fd);
                }
            }
            else {
                int fd = events[i].data.fd;
                Connection_t *conn = find_connection_by_fd(conn_manager, fd);

                if(conn == NULL || conn->client_fd == -1) {
                    char *msg = "Error - find connection by fd failed\n";
                    write(1, msg, 36);
                    continue;    
                } 

                if(fd == conn->client_fd && (events[i].events & EPOLLIN)){
                    if(read_socket(conn, 1) == 0){
                        if(conn->client_buffer && conn->client_buffer_len > 0){
							if(conn->flag >= 1){
								int8_t sent = send_buffer(conn, conn->remote_server_fd);
                                if(sent == -1){
                                    int idx = find_idx_by_fd(conn_manager, fd);
                                    if(idx != -1) remove_connection(conn_manager, idx);
                                    continue;
                                }
								
								struct epoll_event ev_remote;
                                ev_remote.data.fd = conn->remote_server_fd;
                                if(conn->client_buffer_len > 0) {
                                    ev_remote.events = EPOLLIN | EPOLLOUT;
                                } else {
                                    ev_remote.events = EPOLLIN;
                                }
                                epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev_remote);
                                continue;
							}
							
							if (conn->state == 0){

								if (strstr(conn->client_buffer, "\r\n\r\n") == NULL) {
        							continue; 
    							}	
								
								if(conn->req != NULL) conn->req = free_request(conn->req);
								conn->req = init_http_request();
								
								if(conn->res != NULL) conn->res = free_response(conn->res);
								conn->res = init_http_response();

	                            int result = read_buffer(cache, conn, 2);
	                            if(result == -1) {
                                    int idx = find_idx_by_fd(conn_manager, fd);
                                    if(idx != -1) remove_connection(conn_manager, idx);
                                    continue;
                                }
	                            if(result == 1) {
	                                continue;
	                            }
								if(result == 2) {
									struct epoll_event ev_remote;
									ev_remote.events = EPOLLIN | EPOLLOUT;
									ev_remote.data.fd = conn->remote_server_fd;
									epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev_remote);
                                    continue;
								}
								if(conn->req && conn->req->method && strcmp(conn->req->method, "CONNECT") == 0) {
									char *headers_end = strstr(conn->client_buffer, "\r\n\r\n");
									if (headers_end) {
										headers_end += 4; 
										size_t headers_len = headers_end - conn->client_buffer;
										
										if (conn->client_buffer_len > headers_len) {
											size_t remaining = conn->client_buffer_len - headers_len;
											memmove(conn->client_buffer, headers_end, remaining);
											conn->client_buffer_len = remaining;
										} else {
											conn->client_buffer_len = 0;
										}
									} else {
										conn->client_buffer_len = 0;
									}
									
									char *host_port = conn->req->path;
									if(host_port) {
										char hostname[256];
										int port = 443;  
										char *colon = strchr(host_port, ':');
										
										if(colon) {
											int host_len = colon - host_port;
											if(host_len < 256) {
												strncpy(hostname, host_port, host_len);
												hostname[host_len] = '\0';
												port = atoi(colon + 1);
											}
										} else {
											strncpy(hostname, host_port, 255);
											hostname[255] = '\0';
										}
										
										char *ip = get_ip_from_host(hostname);
										if(ip) {
											struct sockaddr_in remote_addr;
											remote_addr.sin_family = AF_INET;
											remote_addr.sin_addr.s_addr = inet_addr(ip);
											remote_addr.sin_port = htons(port);
											
											if(connect(conn->remote_server_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
												if(errno != EINPROGRESS) {
													int idx = find_idx_by_fd(conn_manager, conn->client_fd);
													if(idx != -1) remove_connection(conn_manager, idx);
													continue;
												}
												conn->state = 2;  
											} else {
												conn->state = 3;  
											}

											conn->flag = 0;
											struct epoll_event ev_remote;
											ev_remote.events = EPOLLIN | EPOLLOUT;
											ev_remote.data.fd = conn->remote_server_fd;
											epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev_remote);
											

										}
									}
								}
								else {
									const char* host = get_header(conn->req->headers, conn->req->header_count, "Host");
									if(host){
										char *ip = get_ip_from_host(host);
										if(ip) {
											struct sockaddr_in remote_addr;
											remote_addr.sin_family = AF_INET;
											remote_addr.sin_addr.s_addr = inet_addr(ip);
											remote_addr.sin_port = htons(80);
											
											if(connect(conn->remote_server_fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
												if(errno != EINPROGRESS) {
													int idx = find_idx_by_fd(conn_manager, conn->client_fd);
													if(idx != -1) remove_connection(conn_manager, idx);
													continue;
												}
												conn->state = 2;
											} else {
												conn->state = 3;
											}
											
											conn->flag = 0;
											struct epoll_event ev_remote;
											ev_remote.events = EPOLLIN | EPOLLOUT;
											ev_remote.data.fd = conn->remote_server_fd;
											epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev_remote);
										}
									}
								}
			
                            }
							else if(conn->state >= 2){
								struct epoll_event ev_remote;
								ev_remote.events = EPOLLIN | EPOLLOUT;
								ev_remote.data.fd = conn->remote_server_fd;
								epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev_remote);
							}
                        }
                    }
		    
					else {
                        int idx = find_idx_by_fd(conn_manager, fd);
                        if(idx != -1) remove_connection(conn_manager, idx);
                        continue;

                    }

                }
				else if(fd == conn->client_fd && (events[i].events & EPOLLOUT)) {
					int8_t sent = send_buffer(conn, conn->client_fd);
					if(sent == -1){
						int idx = find_idx_by_fd(conn_manager, conn->client_fd);
						if(idx != -1) remove_connection(conn_manager, idx);
						continue;
					}
					if(sent == 0 && conn->remote_server_buffer_len == 0) {
						if(conn->flag == 1) conn->flag = 2;
						
						struct epoll_event ev;
			            ev.events = EPOLLIN;
			            ev.data.fd = conn->client_fd;
			            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev);
			            
			            ev.events = EPOLLIN;
			            ev.data.fd = conn->remote_server_fd;
			            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev);
			            
			        } else {
			            struct epoll_event ev_mod;
			            ev_mod.events = EPOLLIN | EPOLLOUT;
			            ev_mod.data.fd = conn->client_fd;
			            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev_mod);
			        }	
				        
				}

                else if(fd == conn->remote_server_fd && (events[i].events & EPOLLOUT)) {
					int error = 0;
				    socklen_t errlen = sizeof(error);
				    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen) < 0 || error != 0) {
				        int idx = find_idx_by_fd(conn_manager, fd);
				        if(idx != -1) remove_connection(conn_manager, idx);
				        continue;
				    }
                    if (conn->state == 2 || (conn->state == 3 && conn->flag == 0)) { 
        				conn->state = 3;
        
		        		if (conn->req && conn->req->method && strcmp(conn->req->method, "CONNECT") == 0 && conn->flag == 0) {
		            			char *ok_msg = "HTTP/1.1 200 Connection Established\r\n\r\n";
		            			size_t msg_len = strlen(ok_msg);
		            
		           		 	if (conn->remote_server_buffer_cap < msg_len) {
		                			char* relocated_msg = realloc(conn->remote_server_buffer, msg_len);
		                			if(relocated_msg != NULL) { 
		                    				conn->remote_server_buffer = relocated_msg;
		                    				conn->remote_server_buffer_cap = msg_len;
		                			}
							}
							memcpy(conn->remote_server_buffer, ok_msg, msg_len);
							conn->remote_server_buffer_len = msg_len;
							conn->flag = 1;
	
							struct epoll_event ev_client;
							ev_client.events = EPOLLIN | EPOLLOUT;
							ev_client.data.fd = conn->client_fd;
							epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev_client);
		        		}
    		   		}		

				    int8_t sent = send_buffer(conn, conn->remote_server_fd);

					if (sent == -1) {
                        int idx = find_idx_by_fd(conn_manager, fd);
                        if(idx != -1) remove_connection(conn_manager, idx);
                        continue;
                    }
				    
				    struct epoll_event ev_mod;
					ev_mod.data.fd = conn->remote_server_fd;
					if (conn->client_buffer_len > 0) { 
						ev_mod.events = EPOLLIN | EPOLLOUT; 
					} else {
						ev_mod.events = EPOLLIN; 
					}
					epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev_mod);
                }
                else if(fd == conn->remote_server_fd && (events[i].events & EPOLLIN)) {
                    if(read_socket(conn, 2) == 0){
						if(conn->flag == 0) read_buffer(cache, conn, 1);
						
                        if(conn->remote_server_buffer && conn->remote_server_buffer_len > 0){
							
							int8_t sent = send_buffer(conn, conn->client_fd);
                            if(sent == -1) {
                                int idx = find_idx_by_fd(conn_manager, fd);
                                if(idx != -1) remove_connection(conn_manager, idx);
                                continue;
                            }
							
							struct epoll_event ev;
                            ev.data.fd = conn->client_fd; 
                            if(conn->remote_server_buffer_len > 0) {
                                ev.events = EPOLLIN | EPOLLOUT; 
                            } else {
                                ev.events = EPOLLIN;
                            }
                            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev);						
                        }
                    } 
					else{
				    	int idx = find_idx_by_fd(conn_manager, fd);
		    			if(idx != -1) remove_connection(conn_manager, idx);
		    			continue;
		    
		    		}
                }
                else if(events[i].events & (EPOLLHUP | EPOLLERR)) {
                    int idx = find_idx_by_fd(conn_manager, fd);
                    if(idx != -1) {
                        remove_connection(conn_manager, idx);
                    }
                }
            }
        }
    }

    close(epfd);
    close(server->socket_fd);
    free(server);
    free_connection_manager(conn_manager);
    return 0;
}
