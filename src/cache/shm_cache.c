#include "shm_cache.h"
#include <string.h>

int8_t init_cache(Cache_t *cache);

int8_t content_filtering(const char *url, const char *data);


int8_t add_cache(Cache_t *cache, const char *url, const char *data){
	uint16_t id = hash(url);
	time_t time;
	CacheEntry_t *entry = cache->entries[id];
	uint8_t x = 0;
	time_t y;
	uint8_t z = 0;
	while(1){
		if(entry->next == cache->entries[id]){
			break;
		}
		if(valid){
			if(strcmp(entry->url, url) == 0){
                        	strcpy(entry->data, data);
                        	entry->timestamp = time(&time);
				return 0;
	                }
		       	x++;
			if(entry->timestamp < time(&y)){
				y = entry->timestamp;
				z = x;
			}		
			entry = entry->next;	
		} 
		else {
			strcpy(entry->url, url);
			strcpy(entry->data, data);
			entry->timestamp = time(&time);
			entry->valid = 1;
			if(id < TABLE_SIZE){
				entry->next = cache->entries[id+1];

			}
			else {
				entry->next = cache->entries[0];
			}
			cache->count++;
			return 0;
		}
		
	}
	strcpy(cache->entries[z].url, url);
	strcpy(cache->entries[z].data, data);
	entry->timestamp = time(&time);

	return 0;
	
}

uint16_t hash(const char *url){
	unsigned int sum = 0;
	while (*url != "\0"){
		sum = (int)*url * 11 + sum;
		url++;
	}
	return sum % TABLE_SIZE;
}

