#ifndef HTTP_PARSE
#define HTTP_PARSE

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
http_response_t* response_parser(http_response_t *response, char *buffer);
http_request_t* request_parser(http_request_t *request, char *buffer);
void parser_query_string(char *path, char **query_string, char **clean_path);

#endif
