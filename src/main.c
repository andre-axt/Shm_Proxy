#include "server.h"
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>  

#define MAX_CONNECTIONS 100

Socket_t *server;
int epfd;
ConnectionManager_t *conn_manager;

void stop_server(int sig){
    if(server && server->socket_fd != -1){
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
    server->backlog = MAX_CONNECTIONS;  
	
    epfd = epoll_create1(0);
    
    switch(create_server(server)){
        case -1: {
            free(server);
            char *msg = "Error - socket create failed\n";
            write(1, msg, 29);
            return 1;
        }
        case 1: {
            close(server->socket_fd);
            free(server);
            char *msg = "Error - bind socket failed\n";
            write(1, msg, 27);
            return 1;
        }
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
                        continue; 
                    }
                    
                    conn->remote_server_fd = socket(AF_INET, SOCK_STREAM, 0);
                    if(conn->remote_server_fd < 0) {
                        int idx = find_idx_by_fd(conn_manager, client_fd);
                        if(idx != -1) {
                            remove_connection(conn_manager, idx);
                        }
                        continue;
                    }
                    
                    set_nonblocking(conn->remote_server_fd);
                }
            }
            else {
                int fd = events[i].data.fd;
                Connection_t *conn = find_connection_by_fd(conn_manager, fd);

                if(conn == NULL){
                    char *msg = "Error - find connection by fd failed\n";
                    write(1, msg, 36);
                    continue;    
                } 

                if(fd == conn->client_fd && (events[i].events & EPOLLIN)){
                    if(read_socket(conn, 1) == 0){
                        if(conn->buffer && conn->buffer_len > 0){
                            int result = read_buffer(conn);
                            
                            if(result == -1) {
                                char *msg = "Error - read buffer failed\n";
                                write(1, msg, 28);
                                continue;
                            }
                            else if(result == 2) {  
                                if(conn->req && conn->req->method && 
                                   strcmp(conn->req->method, "CONNECT") == 0) {
                                    
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
                                            strcpy(hostname, host_port);
                                        }
                                        
                                        char *ip = get_ip_from_host(hostname);
                                        if(ip) {
                                            struct sockaddr_in remote_addr;
                                            remote_addr.sin_family = AF_INET;
                                            remote_addr.sin_addr.s_addr = inet_addr(ip);
                                            remote_addr.sin_port = htons(port);
                                            
                                            if(connect(conn->remote_server_fd, (struct sockaddr*)&remote_addr, 
                                                      sizeof(remote_addr)) < 0) {
                                                if(errno != EINPROGRESS) {
                                                    int idx = find_idx_by_fd(conn_manager, conn->client_fd);
                                                    if(idx != -1) remove_connection(conn_manager, idx);
                                                    continue;
                                                }
                                                conn->state = 2;  
                                            } else {
                                                conn->state = 3;  
                                            }
                                            
                                            struct epoll_event ev_remote;
                                            ev_remote.events = EPOLLIN | EPOLLOUT | EPOLLET;
                                            ev_remote.data.fd = conn->remote_server_fd;
                                            epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev_remote);
                                        }
                                    }
                                }
                                else {
                                    const char* host = get_header(conn->req->headers, conn->req->header_count, "Host:");
                                    if(host){
                                        char *ip = get_ip_from_host(host);
                                        if(ip) {
                                            struct sockaddr_in remote_addr;
                                            remote_addr.sin_family = AF_INET;
                                            remote_addr.sin_addr.s_addr = inet_addr(ip);
                                            remote_addr.sin_port = htons(80);
                                            
                                            if(connect(conn->remote_server_fd, (struct sockaddr*)&remote_addr, 
                                                      sizeof(remote_addr)) < 0) {
                                                if(errno != EINPROGRESS) {
                                                    int idx = find_idx_by_fd(conn_manager, conn->client_fd);
                                                    if(idx != -1) remove_connection(conn_manager, idx);
                                                    continue;
                                                }
                                                conn->state = 2;
                                            } else {
                                                conn->state = 3;
                                            }
                                            
                                            struct epoll_event ev_remote;
                                            ev_remote.events = EPOLLIN | EPOLLOUT | EPOLLET;
                                            ev_remote.data.fd = conn->remote_server_fd;
                                            epoll_ctl(epfd, EPOLL_CTL_ADD, conn->remote_server_fd, &ev_remote);
                                        }
                                    }
                                }
                            }
                            else if(result == 1) {  

                                if(conn->buffer && conn->buffer_len > 0) {
                                    int8_t sent = send_buffer(conn, conn->client_fd);
                                    if(sent == -1){
                                        int idx = find_idx_by_fd(conn_manager, conn->client_fd);
                                        if(idx != -1) remove_connection(conn_manager, idx);
                                        continue;
                                    }
                                    while(sent == 1){
                                        sent = send_buffer(conn, conn->client_fd);
                                    }
									conn->buffer_len = 0;
									memset(conn->buffer, '\0', sizeof(conn->buffer)); 
                                }
                            }
                        }
                    }
                }

                else if(fd == conn->remote_server_fd && (events[i].events & EPOLLOUT)) {
                    if(conn->state == 2) {  
                        conn->state = 3;  

                        if(conn->req && conn->req->method && 
                           strcmp(conn->req->method, "CONNECT") == 0) {
 
                            char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
                            send(conn->client_fd, response, strlen(response), 0);

                            conn->buffer_len = 0;
                            
                            struct epoll_event ev_mod;
                            ev_mod.events = EPOLLIN | EPOLLET;
                            ev_mod.data.fd = conn->client_fd;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->client_fd, &ev_mod);
                            
                            ev_mod.data.fd = conn->remote_server_fd;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev_mod);
                        } else {
                            if(conn->req != NULL) {
                                int8_t sent = send_buffer(conn, conn->remote_server_fd);
                                while(sent == 1){
                                    sent = send_buffer(conn, conn->remote_server_fd);
                                }
								conn->buffer_len = 0;
								memset(conn->buffer, '\0', sizeof(conn->buffer)); 
                            }
                            
                            struct epoll_event ev_mod;
                            ev_mod.events = EPOLLIN | EPOLLET;
                            ev_mod.data.fd = conn->remote_server_fd;
                            epoll_ctl(epfd, EPOLL_CTL_MOD, conn->remote_server_fd, &ev_mod);
                        }
                    }
                }
                else if(fd == conn->remote_server_fd && (events[i].events & EPOLLIN)) {
                    if(read_socket(conn, 2) == 0){
                        if(conn->buffer && conn->buffer_len > 0){

							int8_t sent = send_buffer(conn, conn->client_fd);
							if(sent == -1){
								int idx = find_idx_by_fd(conn_manager, conn->client_fd);
								if(idx != -1) remove_connection(conn_manager, idx);
								continue;
							}
							while(sent == 1){
								sent = send_buffer(conn, conn->client_fd);
							}
							conn->buffer_len = 0;
							memset(conn->buffer, '\0', sizeof(conn->buffer)); 
                        }
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
