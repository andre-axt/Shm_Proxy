#include "http_parser.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

http_request_t* init_http_request(){
        http_request_t *req = malloc(sizeof(http_request_t));
        req->method = NULL;
        req->path = NULL;
        req->version = NULL;
        req->headers = NULL;
        req->header_count = 0;
        req->body = NULL;
        req->body_length = 0;
        return req;

}

http_response_t* init_http_response(){
        http_response_t *res = malloc(sizeof(http_response_t));
        res->status_code = 0;
        res->version = NULL;
        res->headers = NULL;
        res->header_count = 0;
        res->body = NULL;
        res->body_length = 0;
        return res;
        
}

void free_request(http_request_t *req){
        free(req->method);
        free(req->path);
        free(req->query_string);
        free(req->version);
        for (int i = 0; i < req->header_count; i++) {
                free(req->headers[i]);
        }
        free(req->headers);
        free(req->body);
        free(req);
}

void free_response(http_response_t *res){
        free(res->version);
        for (int i = 0; i < res->header_count; i++) {
                free(res->headers[i]);
        }
        free(res->headers);
        free(res->body);
        free(res);
}

char *trim(char *str) {
        while(isspace((unsigned char)*str)) str++;
        if(*str == 0) return str;

        char *end = str + strlen(str) - 1;
        while(end > str && isspace((unsigned char) *end)) end--;
        end[1] = '\0';
        return str;
}

void parse_query_string(char *path, char **query_string, char **clean_path){
        char *qmark = strchr(path, '?');
        if(qmark) {
                *query_string = strdup(qmark + 1);
                *clean_path = strndup(path, qmark - path);
        } else{
                *query_string = NULL;
                *clean_path = strdup(path);
        }
       
}

int8_t parse_headers(char *buffer, char ***headers, int8_t *header_count) {
        char *line = buffer;
        int count = 0;
        char **x_headers = malloc(100 * sizeof(char*));

        while (*line != '\0' && count < 100){
                char *end = strstr(line, "\r\n");
                if (!end) break;
                if (end == line) break;
                int line_len = end - line;
                x_headers[count] = strndup(line, line_len);
                count++;

                line = end + 2;
        }

        *headers = x_headers;
        *header_count = count;
        return 0;
}

char *get_header(char **headers, int header_count, const char *name) {
    for (int i = 0; i < header_count; i++) {
        if (strncasecmp(headers[i], name, strlen(name)) == 0 && headers[i][strlen(name)] == ':') {
            char *value = headers[i] + strlen(name) + 1; 
            return trim(value);
        }
    }
    return NULL;
}

char *parse_chunked_body(char *chunked_data, size_t *body_length) {
        char *result = malloc(1);
        int total_size = 0;
        char *ptr = chunked_data;

        while (*ptr) {
                char *end = strstr(ptr, "\r\n");
                if (!end) break;

                int chunk_size = strtol(ptr, NULL, 16);
                if (chunk_size == 0) break;

                ptr = end + 2;
                result = realloc(result, total_size + chunk_size + 1);
                memcpy(result + total_size, ptr, chunk_size);
                total_size += chunk_size;

                ptr += chunk_size;
                if (*ptr == '\r' && *(ptr+1) == '\n') {
                        ptr += 2;
                }
                
        }

        result[total_size] = '\0';
        *body_length = total_size;
        return result;
}

http_request_t* request_parser(http_request_t *request, char *buffer){
        char *buffer_aux = strdup(buffer);
        char *line = strtok(buffer_aux, "\r\n");

        if (!line){
                free(buffer_aux);
                char msg* = "Error - request parser failed";
                write(1, msg, 29);
                return request;
        } 

        char *method = strtok(line, " ");
        char *full_path = strtok(NULL, " ");
        char *version = strtok(NULL, " ");

        if (method && full_path && version) {
                request->method = strdup(method);
                request->version = strdup(version);

                parse_query_string(full_path, &request->query_string, &request->path);
        }
        
        char *headers_start = strstr(buffer, "\r\n");
        if (headers_start) {
                headers_start += 2;
                char *headers_end = strstr(headers_start, "\r\n\r\n");
                if (headers_end) {
                        int headers_len = headers_end - headers_start;
                        char *headers_buffer = strndup(headers_start, headers_len);
                        parse_headers(headers_buffer, &request->headers, &request->header_count);
                        free(headers_buffer);
                        char *content_length_str = get_header(request->headers, request->header_count, "Content-Length");
                        if (content_length_str) {
                                long unsigned int content_length = (long unsigned int)atoi(content_length_str);
                                char *body_start = headers_end + 4;
                                if (strlen(body_start) >= content_length) {
                                        request->body = strndup(body_start, content_length);
                                }
                        }
                }
        }

        free(buffer_aux);
        return request;
        
}

http_response_t* response_parser(http_response_t* response, char *buffer){
        char *buffer_aux = strdup(buffer);
        char *line = strtok(buffer_aux, "\r\n");

        if(!line){
                free(buffer_aux);
                return response;      
        } 

        char *version = strtok(line, " "); 
        char *status = strtok(NULL, " "); 
        char *reason_phrase = strtok(NULL, " "); 

        if (version && status && reason_phrase){
                response->version = strdup(version);
                response->status_code = atoi(status);
                reason_phrase = trim(reason_phrase);
                response->reason_phrase = strdup(reason_phrase);
        }

        char *headers_start = strstr(buffer, "\r\n");
        if (headers_start) {
                headers_start += 2;
                char *headers_end = strstr(headers_start, "\r\n\r\n");
                if (headers_end) {
                        int headers_len = headers_end - headers_start;
                        char *headers_buffer = strndup(headers_start, headers_len);
                        parse_headers(headers_buffer, &response->headers, &response->header_count);
                        free(headers_buffer);

                        char *body_start = headers_end + 4;
                        long unsigned int content_length = 0;

                        for (int i = 0; i < response->header_count; i++) {
                                if (strncasecmp(response->headers[i], "Content-Length:", 15) == 0) {
                                        char *value = response->headers[i] + 15;
                                        content_length = (long unsigned int)atoi(trim(value));
                                        break;
                                }
                        }

                        int is_chunked = 0;
                        for (int i = 0; i < response->header_count; i++) {
                                if (strncasecmp(response->headers[i], "Transfer-Enconding:", 18) == 0) {
                                        char *value = response->headers[i] + 18;
                                        if (strstr(trim(value), "chunked")) {
                                                is_chunked = 1;
                                                break;
                                        }
                                }
                        }
                        if (is_chunked) {
                                response->body = parse_chunked_body(body_start, &response->body_length);
                        }
                        else if (content_length > 0){
                                if (strlen(body_start) >= content_length) {
                                        response->body = strndup(body_start, content_length);
                                        response->body_length = content_length;
                                }
                        }
                }
        }
        
        free(buffer_aux);
        return response;
}
