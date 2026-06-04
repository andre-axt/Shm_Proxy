#include "shm_cache.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


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
	if (!cache) return NULL;
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

int8_t add_cache(Cache_t *cache, const char *url, const char *data) {
    if (!cache || !url || !data) return -1;
    
    uint16_t id = hash(url);
    CacheEntry_t *head = &cache->entries[id]; 
    
    CacheEntry_t *prev = NULL;
	CacheEntry_t *curr = head;
	while (curr) {
		if (strcmp(curr->url, url) == 0) {
			if (curr->response) free(curr->response);
			curr->response = strdup(data);
			curr->response_len = strlen(data);
			curr->timestamp = time(NULL);
			return 0;
		}
		prev = curr;
		curr = curr->next;
	}

	time_t now = time(NULL);
	CacheEntry_t *oldest = NULL;
	CacheEntry_t *oldest_prev = NULL;
	curr = head;
	prev = NULL;
	while (curr) {
		if (difftime(now, curr->timestamp) > 600) {
			if (curr->response) free(curr->response);
			strncpy(curr->url, url, URL_MAX - 1);
			curr->url[URL_MAX-1] = '\0';
            curr->response = strdup(data);
            curr->response_len = strlen(data);
            curr->timestamp = now;
            return 0;
		}
		if (!oldest || curr->timestamp < oldest->timestamp) {
			oldest = curr;
			oldest_prev = prev;
		}
		prev = curr;
		curr = curr->next;
	}
	
	int count = 0;
	curr = head;
	while (curr) { 
		count++;
		curr = curr->next;
	}
	if (count >= 4 && oldest) {
		if (oldest->response) free(oldest->response);
		strncpy(oldest->url, url, URL, URL_MAX - 1);
		oldest->url[URL_MAX-1] = '\0';
        oldest->response = strdup(data);
        oldest->response_len = strlen(data);
        oldest->timestamp = now;
        return 0;
		
	}

	CacheEntry_t *new_node = malloc(sizeof(CacheEntry_t));
	if (!new_node) return -1;
    strncpy(new_node->url, url, URL_MAX-1);
    new_node->url[URL_MAX-1] = '\0';
    new_node->response = strdup(data);
    new_node->response_len = strlen(data);
    new_node->timestamp = now;
    new_node->next = NULL;

	if (prev) prev->next = new_node;
	else head->next = new_node;

	cache->count++;
	return 0;
}


CacheEntry_t* find_cache(Cache_t *cache, const char *url){
	int16_t id = hash(url);
	CacheEntry_t *entry = &cache->entries[id];
	time_t current_time = time(NULL);
	for(int i = 0; i <= 4 && entry != NULL; i++){
		if(strcmp(entry->url, url) == 0){
			if(difftime(current_time, entry->timestamp) <= 600){
				char *msg = "Success - find cache";
    			write(1, msg, 21);
				return entry;
			}
			else{
				return NULL;
			}
		}
		entry = (CacheEntry_t *) entry->next;
		
	}
	return NULL;
}
