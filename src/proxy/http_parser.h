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
	char *version;
	char **headers;
	int header_count;
	char *body;
	size_t body_length;

} http_request_t;


http_request_t* init_http_request();

#endif
