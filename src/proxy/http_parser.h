#ifndef HTTP_PARSE
#define HTTP_PARSE
#include <stdint.h>
#include <stddef.h>
typedef struct{
	int status_code;
	char *version;
	char **headers;
	int header_count;
	char *body;
	size_t body_length;

} http_response_t;

typedef struct{
	char *method;
	char *path;
	char *query_string;
	char *version;
	char **headers;
	int header_count;
	char *body;
	size_t body_length;

} http_request_t;


http_request_t* init_http_request();
http_response_t* init_http_response();
void free_request(http_request_t *req);
void free_response(http_response_t *res);
char *trim(char *str);
int8_t parse_headers(char *buffer, char **headers, int *header_count);
char *get_header(char **headers, int header_count, const char *name);
void parser_query_string(char *path, char **query_string, char **clean_path);
http_response_t* response_parser(http_response_t *response, char *buffer);
http_request_t* request_parser(http_request_t *request, char *buffer);

#endif
