#ifndef
#define

#include <time.h>

#define CACHE_SIZE 10
#define URL_MAX	256
#define DATA_MAX 1024

typedef struct {
	char url[URL_MAX];
	char data[DATA_MAX];
	time_t timestamp;
	int valid;
} CacheEntry_t;

typedef struct {
	CacheEntry_t entries[CACHE_SIZE];
	int count;

} Cache_t;

void init_cache(Cache_t *cache);
int content_filtering(const char *url, const char *data);
void add_cache(Cache_t *cache, const char *url, const char *data);

#endif
