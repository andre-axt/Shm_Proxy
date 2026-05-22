#include "shm_cache.h"
#include <string.h>


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
	time_t time;
	CacheEntry_t *entry = cache->entries[id];
	uint8_t aux = 0;
	time_t y = entry->timestamp;
	uint8_t z = 0;
	while(entry && aux <= 5){
		if(strcmp(entry->url, url) == 0){
            strcpy(entry->response, data);
			entry->response = strlen(data);
            entry->timestamp = time(&time);
			return 0;
	    }
		if(entry->timestamp < time(&y)){
			y = entry->timestamp;
			z = aux;
		}	
		aux++;

		entry = entry->next;
 							
	}
	
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


http_response_t* find_cache(Cache_t *cache, const char *url){
	hash_url = hash(url);
	time_t time = time(&time);
	for(int i = 0; i <= 4; i++){
		if(cache->entries[hash_url + i].url = url){
			int32_t dif_time = cache->entries[hash_url + i].timestamp - time;
			if(dif_time <= 600){
				return NULL;
			}
			return cache->entries[hash_url + i].response;		
		}
		
	}
	return NULL;
}
