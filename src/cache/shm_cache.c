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
		cache->entries[i].url[0] = '\0';
        cache->entries[i].response = NULL;
        cache->entries[i].response_len = 0;
        cache->entries[i].timestamp = 0;
        cache->entries[i].next = NULL;
	}

	return cache;

}

int8_t content_filtering(const char *url, const char *data);


int8_t add_cache(Cache_t *cache, const char *url, const char *data) {
    if (!cache || !url || !data) return -1;
    
    uint16_t id = hash(url);
    CacheEntry_t *entry = &cache->entries[id]; 
    
    uint8_t aux = 0;
    CacheEntry_t *oldest_entry = NULL;
    time_t oldest_time = time(NULL);

    CacheEntry_t *current = entry;
    while (current && aux < 4) {  
        if (strcmp(current->url, url) == 0) {

            if (current->response) {
                free(current->response);
            }
            current->response = strdup(data); 
            current->response_len = strlen(data);
            current->timestamp = time(NULL);
            return 0;
        }
        
        if (current->timestamp < oldest_time) {
            oldest_time = current->timestamp;
            oldest_entry = current;
        }
        
        aux++;
        current = (CacheEntry_t*)current->next;
    }
	
   
    if (oldest_entry) {
		entry = oldest_entry;
		if (entry->response) {
			free(entry->response);
		}
    } 
	else {
		return -1;  
	}
    
    strncpy(entry->url, url, URL_MAX - 1);
    entry->url[URL_MAX - 1] = '\0';
    entry->response = strdup(data);
    entry->response_len = strlen(data);
    entry->timestamp = time(NULL);
    entry->next = NULL;  
    
    cache->count++;
    return 0;
}


CacheEntry_t* find_cache(Cache_t *cache, const char *url){
	int16_t id = hash(url);
	CacheEntry_t *entry = &cache->entries[id];
	time_t current_time = time(NULL);
	for(int i = 0; i <= 4; i++){
		if(strcmp(entry->url, url) == 0){
			if(difftime(current_time, entry->timestamp) >= 600){
				return entry;
			}
			else{
				return NULL;
			}
		}
		entry = entry->next;
		
	}
	return NULL;
}
