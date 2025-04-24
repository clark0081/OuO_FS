#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>
#include "block.h"
#include "lrucache.h"


/*
typedef struct inode_info{
    int isDirty;
    int inodeNum;
    struct inode_info *next;
    struct inode_info *prev;
    struct inode *val; 
} INODE_INFO;

*/

/*
typedef struct block_info{
    int isDirty;
    int blockNum;
    struct block* prev;
    struct block* next;
    // BLOCKSIZE = SECTORSIZE = 512
    char data[BLOCKSIZE];
} BLOCK_INFO;
*/

INODE_INFO* inode_head;  
INODE_INFO* inode_tail;  
BLOCK_INFO* block_head;  
BLOCK_INFO* block_tail;  
int inode_cache_len;  
int block_cache_len;  

void init_lru(){
    inode_cache_len = 0;
    block_cache_len = 0;
    inode_head = (INODE_INFO*)malloc(sizeof(INODE_INFO));
    inode_head = (INODE_INFO*)malloc(sizeof(INODE_INFO));
    block_head = (BLOCK_INFO*)malloc(sizeof(BLOCK_INFO));
    block_head = (BLOCK_INFO*)malloc(sizeof(BLOCK_INFO));

    inode_head->isDirty = 0;
    inode_head->inodeNum = -1;
    inode_head->next = NULL;
    inode_head->prev = NULL;
    inode_head->val = NULL;
    inode_tail->isDirty = 0;
    inode_tail->inodeNum = -1;
    inode_tail->next = NULL;
    inode_tail->prev = NULL;
    inode_tail->val = NULL;

    inode_head->next = inode_tail;
    inode_tail->prev = inode_head;

    block_head->isDirty = 0;
    block_head->blockNum = -1;
    block_head->next = NULL;
    block_head->prev = NULL;
    block_tail->isDirty = 0;
    block_tail->blockNum = -1;
    block_tail->next = NULL;
    block_tail->prev = NULL;

    block_head->next = block_tail;
    block_tail->prev = block_head;
}
void dequeue_inode(){
    if (inode_head->next == inode_tail) {
        return;
    }
    INODE_INFO* temp = inode_head->next;
    inode_head->next = temp->next;
    temp->next->prev = inode_head;
    inode_cache_len--;
    temp->next = NULL;
    temp->prev = NULL;
}
void enqueue_inode(INODE_INFO* x){
    if (x == NULL) return;
    x->next = inode_tail;
    x->prev = inode_tail->prev;
    inode_tail->prev->next = x;
    inode_tail->prev = x;
    inode_cache_len++;
}
void remove_queue_inode(INODE_INFO* x){
    if (x == NULL || x == inode_head || x == inode_tail) return; 
    x->prev->next = x->next;
    x->next->prev = x->prev;
    x->next = NULL;
    x->prev = NULL;
    inode_cache_len--;
}

void dequeue_block(){
    // pop head next
    if (block_head->next == block_tail) {
        return;
    }
    BLOCK_INFO* temp = block_head->next;
    block_head->next = temp->next;
    temp->next->prev = block_head;
    block_cache_len--;
    temp->next = NULL;
    temp->prev = NULL;
}
void enqueue_block(BLOCK_INFO* x){
    // add to tail
    if (x == NULL) return;
    x->next = block_tail;
    x->prev = block_tail->prev;
    block_tail->prev->next = x;
    block_tail->prev = x;
    block_cache_len++;
}
void remove_queue_block(BLOCK_INFO* x){
    if (x == NULL || x == inode_head || x == inode_tail) return; 
    x->prev->next = x->next;
    x->next->prev = x->prev;
    x->next = NULL;
    x->prev = NULL;
    block_cache_len--;
}
void clear_inode_cache(){
    INODE_INFO* current = inode_head->next;

    while (current != NULL && current != inode_tail) {
        INODE_INFO* temp = current;
        current = current->next;
        free(temp->val);
        free(temp);
        inode_cache_len--;
    }
    inode_head->next = inode_tail;
    inode_tail->prev = inode_head;
}
void clear_block_cache(){
    BLOCK_INFO* current = block_head->next;

    while (current != NULL && current != block_tail) {
        BLOCK_INFO* temp = current;
        current = current->next;
        free(temp);
        block_cache_len--;
    }
    block_head->next = inode_tail;
    block_tail->prev = inode_head;
}
/*
void insert_inode_hashTable(int , INODE_INFO* );
void insert_block_hashTable(int , BLOCK_INFO* );
INODE_INFO* search_inode_hashTable(int);
BLOCK_INFO* search_block_hashTable(int);
void delete_inode_hashTable(int);
void delete_block_hashTable(int);
*/

BLOCK_INFO* get_block_lru(int blockNum){
    BLOCK_WRAP* res = search_block_hashTable(blockNum);
    if(res == NULL) {
        return block_head;
    }else{
        remove_queue_block(res->value);
        // add to tail
        enqueue_block(res->value);
        return res->value;
    }
}
void set_block_lru(int blockNum, BLOCK_INFO* block){
    BLOCK_WRAP* res = search_block_hashTable(blockNum);
    if(res == NULL) {
        // evict shoud reduce the cache size by 1
        evict_block();
        enqueue_block(block);
        insert_block_hashTable(blockNum, block);
    }else{
        delete_block_hashTable(blockNum);
        remove_queue_block(block);
        enqueue_block(block);
        insert_block_hashTable(blockNum, block);
    }
}
   
INODE_INFO* get_inode_lru(int inodeNum){
    INODE_WRAP* res = search_inode_hashTable(inodeNum);
    if(res == NULL) {
        return inode_head;
    }else{
        remove_queue_inode(res->value);
        // add to tail
        enqueue_inode(res->value);
        return res->value;
    }
}
void set_inode_lru(int inodeNum, INODE_INFO* inode){
    INODE_WRAP* res = search_inode_hashTable(inodeNum);
    if(res == NULL) {
        // evict shoud reduce the cache size by 1
        evict_inode();
        enqueue_inode(inode);
        insert_inode_hashTable(inodeNum, inode);
    }else{
        delete_inode_hashTable(inodeNum);
        remove_queue_inode(inode);
        enqueue_inode(inode);
        insert_inode_hashTable(inodeNum, inode);
    }
}
void evict_block(){
    // check whether cache is full
    if(block_cache_len >= BLOCK_CACHESIZE) {
        BLOCK_INFO* block_evict = block_head->next;
        if(block_evict == block_tail) {
            return;
        }
        // remove from hash table
        // write back
        int blockNum = block_evict->blockNum;
        if(block_evict->isDirty == 1) {
            int sig = WriteSector(blockNum, (void*)(block_evict->data));
            if(sig != 0) {
                printf("in evict_block()..., WriteSector Error.\n");
                return;
            }   
        }
        delete_block_hashTable(blockNum);
        remove_queue_block(block_evict);
        free(block_evict);
        block_evict = NULL;
    }
}
void evict_inode(){
    if(inode_cache_len >= INODE_CACHESIZE) {
        INODE_INFO* inode_evict = inode_head->next;
        if(inode_evict == inode_tail) {
            return;
        }
        // remove from hash table
        // write back
        int inodeNum = inode_evict->inodeNum;
        int blockNum = INODE_TO_BLOCK(inodeNum);
        int offset = INODE_IN_BLOCK_ADDR(inodeNum);
        BLOCK_INFO* tmp_block  = get_block(blockNum); // TODO
        memcpy(tmp_block->data + offset, inode_evict->val, sizeof(struct inode));
        tmp_block->isDirty = 1;
        delete_inode_hashTable(inodeNum);
        remove_queue_inode(inode_evict);
        free(inode_evict->val);
        free(inode_evict);
        inode_evict = NULL;
    }
}
