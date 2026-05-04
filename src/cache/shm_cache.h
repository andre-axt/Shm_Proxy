#ifndef	SHM_CACHE
#define	SHM_CACHE

#include <time.h>
#include <stdint.h>
#include "http_parser.h"

#define TABLE_SIZE 10
#define URL_MAX	256
#define DATA_MAX 1024

typedef struct {
	char url[URL_MAX];
	http_response_t response;
	time_t timestamp;
	struct CacheEntry_t *next;
} CacheEntry_t;

typedef struct {
	CacheEntry_t entries[TABLE_SIZE];
	uint16_t count;

} Cache_t;

int8_t init_cache(Cache_t *cache);
uint16_t hash(const char *url);
int8_t content_filtering(const char *url, const char *data);
int8_t add_cache(Cache_t *cache, const char *url, const char *data);
CacheEntry_t* find_cache(Cache_t *cache, const char *url);

#endif
