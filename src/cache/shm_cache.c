#include "shm_cache.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>


uint16_t hash(const char *url){
        unsigned int sum = 0;

        while (*url != '\0'){
                sum = (int) *url * 11 + sum;
                url++;

        }

        return sum % TABLE_SIZE;
}

Cache_t* init_cache(){
	Cache_t* cache = malloc(sizeof(Cache_t));
	cache->count = 0;
	for (int i = 0; i < TABLE_SIZE; i++) {
		cache->entries[i] = NULL;
	}

	return cache;

}

int8_t content_filtering(const char *url, const char *data);


int8_t add_cache(Cache_t *cache, const char *url, const char *data){
	uint16_t id = hash(url);
	CacheEntry_t entry = cache->entries[id];
	uint8_t aux = 0;
	time_t time = entry->timestamp;
	CacheEntry_t *aux_entry;
	while(entry && aux <= 4){
		if(strcmp(entry->url, url) == 0){
            strcpy(entry->response, data);
			entry->response = data;
            entry->timestamp = time(NULL);
			return 0;
	    }
		if(difftime(entry->timestamp, time) < 0){
			y = entry->timestamp;
			aux_entry = entry;
		}	
		aux++;

		entry = entry->next;
 							
	}
	aux_entry->timestamp = time(NULL);
	aux_entry->valid = 1;
	aux_entry->response = data;
	if(id < TABLE_SIZE){
		entry->next = cache->entries[id+1];

	}
	else {
		entry->next = cache->entries[0];
	}
	
	cache->count++;

	return 0;
	
}


char* find_cache(Cache_t *cache, const char *url){
	int16_t id = hash(url);
	CacheEntry_t *entry = cache->entries[id];
	time_t time = time(NULL);
	for(int i = 0; i <= 4; i++){
		if(strcmp(entry->url, url) == 0){
			if(difftime(time, entry->timestamp) >= 600){
				return entry->response;
			}
			else{
				return NULL;
			}
		}
		entry = entry->next;
		
	}
	return NULL;
}
