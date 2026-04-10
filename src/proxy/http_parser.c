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


