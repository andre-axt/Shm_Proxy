#include "http_parse.h"
#include <stdlib.h>

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
        http_response_t *res = malloc(sizeof(http_response_t);
        res->status_code = 0;
        res->version = NULL;
        res->headers = NULL;
        res->header_count = 0;
        res->body = NULL;
        res->body_length = 0;
        return res;
        
}

http_request_t* request_parser(http_request_t *request, char *buffer){
        buffer_len = strlen(buffer);
        int c = 0;
        char *buffer_aux = malloc(sizeof(buffer_len));
        int8_t aux1 = 0;
        int8_t aux2 = 0;
        while(c = ' '){
                c = buffer[aux1];
                aux1++;
        }
        while(c != '\0' || c != ' '){
                c = buffer[aux2 + aux1];
                buffer_aux[aux2] = c;
                aux2++;
        }
        memcpy(request->method, buffer_aux, sizeof(aux2));
        request->method[[sizeof(buffer_aux) - 1] = '\0';
        aux2++;
        while(c = ' '){
                c = buffer[aux2];
                aux2++;
        }
        aux1 = aux2;
        aux2 = 0;
        memset(buffer_aux, 0, aux1);
        while(c != '\0' || c != ' '){
                c = buffer[aux2 + aux1];
                buffer_aux[aux2] = c;
                aux2++;
        }
        memcpy(request->path, buffer_aux, sizeof(aux2));
        request->path[[sizeof(buffer_aux) - 1] = '\0';
        free(buffer_aux);
}
