#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "block.h"
#include "lrucache.h"

int NUM_BLOCKS;
int NUM_INODES;
int NUM_BLOCKS_FOR_INODES;
unsigned char* block_bitmap = NULL;
unsigned char* inode_bitmap = NULL;
INODE_WRAP* inode_hashtable[HASH_TABLE_SIZE] = {0}; 
BLOCK_WRAP* block_hashtable[HASH_TABLE_SIZE] = {0}; 

struct inode_info* get_use_inode(int inodeNum){
    INODE_INFO* result  = get_inode_lru(inodeNum);
    if(result->inodeNum == -1){
        TracePrintf(1, "@@@@@\n");
        INODE_INFO* res = (INODE_INFO*)malloc(sizeof(INODE_INFO));
        int blockNum = INODE_TO_BLOCK(inodeNum);
        BLOCK_INFO* tempBlock = get_block(blockNum);
        int offset = INODE_IN_BLOCK_ADDR(inodeNum);
        //struct inode* myInode = (struct inode*)(tempBlock->data + offset);
        struct inode* myInode = malloc(sizeof(struct inode));
        memcpy(myInode, tempBlock->data + offset, sizeof(struct inode));
        res->next = NULL;
        res->prev = NULL;
        res->val = myInode;
        res->isDirty = 0;
        res->inodeNum = inodeNum;
        if(res->val->type == INODE_FREE){
            free(res->val);
            free(res);
            return NULL;
        }
        set_inode_lru(inodeNum, res);
        return get_inode_lru(inodeNum);
    }
    else{
        return result;
    }
}
struct inode_info* get_new_inode(){
    int inodeNum = find_free(inode_bitmap, NUM_INODES);
    if(inodeNum == -1){
        return NULL;
    }
    INODE_INFO* res = (INODE_INFO*)malloc(sizeof(INODE_INFO));
    int blockNum = INODE_TO_BLOCK(inodeNum);
    BLOCK_INFO* tempBlock = get_block(blockNum);
    int offset = INODE_IN_BLOCK_ADDR(inodeNum);
    //struct inode* myInode = (struct inode*)(tempBlock->data + offset);
    struct inode* myInode = malloc(sizeof(struct inode));
    memcpy(myInode, tempBlock->data + offset, sizeof(struct inode));
    res->next = NULL;
    res->prev = NULL;
    res->val = myInode;
    res->isDirty = 0;
    res->inodeNum = inodeNum;
    set_bitmap_used(inode_bitmap, inodeNum, NUM_INODES);
    set_inode_lru(inodeNum, res);
    return get_inode_lru(inodeNum);
}

BLOCK_INFO* get_block(int blockNum) {
    // check LRU
    BLOCK_INFO* result  = get_block_lru(blockNum);
	if(result->blockNum == -1) {
		BLOCK_INFO* res = (BLOCK_INFO*)malloc(sizeof(BLOCK_INFO));
        ReadSector(blockNum, (void*)(res->data));
        res->isDirty = 0;
        res->blockNum = blockNum;
        res->next = NULL;
        res->prev = NULL;
        set_block_lru(blockNum, res);
        return get_block_lru(blockNum);
	}else{
		return result;
	}
}

int hash_key(int key) {
    return key % HASH_TABLE_SIZE;
}

void set_bitmap_used(unsigned char* bitmap, int num, int len) {
    if(num < len)
        bitmap[num / 8] |= (1 << (num % 8));
}

void set_bitmap_free(unsigned char* bitmap, int num, int len) {
    if(num < len)
        bitmap[num / 8] &= ~(1 << (num % 8));
}
int is_free(unsigned char* bitmap, int num, int len) {
    if(num < len)
        return !(bitmap[num / 8] & (1 << (num % 8)));
    return -1;
}
int find_free(unsigned char* bitmap, int len) {
    for (int i = 0; i < len; i++) {
        int byte_index = i / 8;
        int bit_offset = i % 8;
        if ((bitmap[byte_index] & (1 << bit_offset)) == 0) {
            return i;
        }
    }
    return -1;
}

void init_inode_block(){
    
    TracePrintf(0, "1\n");
    INODE_INFO* inode_header = (INODE_INFO*)malloc(sizeof(INODE_INFO));
    TracePrintf(0, "11\n");
    int blockNum = INODE_TO_BLOCK(0);
    TracePrintf(0, "blockNum %d\n",blockNum);
    BLOCK_INFO* tempBlock = get_block(blockNum);
    TracePrintf(0, "13\n");
    int offset = INODE_IN_BLOCK_ADDR(0);
    TracePrintf(0, "14\n");
    struct inode* temp = malloc(sizeof(struct inode));
    TracePrintf(0, "15\n");
    memcpy(temp, tempBlock->data + offset, sizeof(struct inode));
    TracePrintf(0, "2\n");
    inode_header->next = NULL;
    inode_header->prev = NULL;
    inode_header->val = temp;
    inode_header->isDirty = 0;
    inode_header->inodeNum = 0;
    struct fs_header* myHeader = (struct fs_header*)(inode_header->val);
    TracePrintf(0, "3\n");
    // init hasg table to null
    memset(inode_hashtable, 0, sizeof(inode_hashtable));
    memset(block_hashtable, 0, sizeof(block_hashtable));
    
    
    NUM_INODES = myHeader->num_inodes;
    NUM_BLOCKS = myHeader->num_blocks;
    NUM_BLOCKS_FOR_INODES = (NUM_INODES * INODESIZE + BLOCKSIZE - 1) / BLOCKSIZE;
    free(inode_header->val);
    free(inode_header);


    int block_bitmap_size = (NUM_BLOCKS + 7) / 8;
    int inode_bitmap_size = (NUM_INODES + 7) / 8;
    block_bitmap = (unsigned char*)malloc(block_bitmap_size);
    inode_bitmap = (unsigned char*)malloc(inode_bitmap_size);
    memset(block_bitmap, 0, block_bitmap_size);
    memset(inode_bitmap, 0, inode_bitmap_size);
    // boot block
    set_bitmap_used(block_bitmap,0,NUM_BLOCKS);
    // fs_header
    set_bitmap_used(inode_bitmap,0,NUM_INODES);


    int i;
    // set block list for inodes used
    for(i = 1; i <= NUM_BLOCKS_FOR_INODES ; i++){
        set_bitmap_used(block_bitmap,i,NUM_BLOCKS);
    }
    // check all inodes
    for(i = 1; i < NUM_INODES; i++){
        INODE_INFO* res = (INODE_INFO*)malloc(sizeof(INODE_INFO));
        int blockNum = INODE_TO_BLOCK(i);
        BLOCK_INFO* tempBlock = get_block(blockNum);
        int offset = INODE_IN_BLOCK_ADDR(i);
        struct inode* temp = malloc(sizeof(struct inode));
        memcpy(temp, tempBlock->data + offset, sizeof(struct inode));
        res->next = NULL;
        res->prev = NULL;
        res->val = temp;
        res->isDirty = 0;
        res->inodeNum = i;
	    if(temp->type != INODE_FREE){
            set_bitmap_used(inode_bitmap,i,NUM_INODES);
	        int j = 0;
	        // direct blocks
	        while(j < NUM_DIRECT && j * BLOCKSIZE < temp->size){
                if(temp->direct[j] > NUM_BLOCKS_FOR_INODES && temp->direct[j] < NUM_BLOCKS){
		            set_bitmap_used(block_bitmap,temp->direct[j],NUM_BLOCKS);
                }
                else{
                    printf("in init...blockNum is out of range\n");
                }
		        j++;		    
	        }
	        if(j * BLOCKSIZE < temp->size){
		        int* indirect_block = (int*)(get_block(temp->indirect)->data);
		        while(j * BLOCKSIZE < temp->size){
                    int indirect_blockNum = indirect_block[j - NUM_DIRECT];
                    if(indirect_blockNum > NUM_BLOCKS_FOR_INODES && indirect_blockNum < NUM_BLOCKS){
		                set_bitmap_used(block_bitmap,indirect_blockNum,NUM_BLOCKS);
                    }
                    else{
                        printf("in init...indirect blockNum is out of range\n");
                    }
		            j++;
		        }
	        }
            set_inode_lru(i, res);
	    }
        else{
            free(res->val);
            free(res);
        }
    }
}

void insert_inode_hashTable(int key, INODE_INFO* val) {
    int index = hash_key(key);
    INODE_WRAP* cur_wrap = inode_hashtable[index];
    while (cur_wrap != NULL) {
        if (cur_wrap->key == key) {
            cur_wrap->value = val;
            return;
        }
        cur_wrap = cur_wrap->next;
    }
    INODE_WRAP* new_wrap = (INODE_WRAP*)malloc(sizeof(INODE_WRAP));
    new_wrap->key = key;
    new_wrap->value = val;
    //put to head
    new_wrap->next = inode_hashtable[index];
    inode_hashtable[index] = new_wrap;
}
void insert_block_hashTable(int key, BLOCK_INFO* val) {
    int index = hash_key(key);
    BLOCK_WRAP* cur_wrap = block_hashtable[index];
    while (cur_wrap != NULL) {
        if (cur_wrap->key == key) {
            cur_wrap->value = val;
            return;
        }
        cur_wrap = cur_wrap->next;
    }
    BLOCK_WRAP* new_wrap = (BLOCK_WRAP*)malloc(sizeof(BLOCK_WRAP));
    new_wrap->key = key;
    new_wrap->value = val;
    //put to head
    new_wrap->next = block_hashtable[index];
    block_hashtable[index] = new_wrap;
}

INODE_WRAP* search_inode_hashTable(int key) {
    int index = hash_key(key);
    INODE_WRAP* current = inode_hashtable[index];
    while (current != NULL) {
        if (current->key == key) {
            return current;  
        }
        current = current->next;
    }
    return NULL;  
}

BLOCK_WRAP* search_block_hashTable(int key) {
    int index = hash_key(key);
    BLOCK_WRAP* current = block_hashtable[index];
    while (current != NULL) {
        if (current->key == key) {
            return current;  
        }
        current = current->next;
    }
    return NULL;  
}

void delete_inode_hashTable(int key) {
    int index = hash_key(key);
    INODE_WRAP* current = inode_hashtable[index];
    INODE_WRAP* prev = NULL;

    while (current != NULL) {
        if (current->key == key) {
            if (prev == NULL) {
                inode_hashtable[index] = current->next;
            } else {
                prev->next = current->next;
            }
            // only free the wrap, not the value
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
void delete_block_hashTable(int key) {
    int index = hash_key(key);
    BLOCK_WRAP* current = block_hashtable[index];
    BLOCK_WRAP* prev = NULL;

    while (current != NULL) {
        if (current->key == key) {
            if (prev == NULL) {
                block_hashtable[index] = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
void clear_inode_block_hashTable() {
    for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
        INODE_WRAP* cur_i = inode_hashtable[i];
        BLOCK_WRAP* cur_b = block_hashtable[i];
        while (cur_i != NULL) {
            INODE_WRAP* temp = cur_i;
            cur_i = cur_i->next;
            free(temp);
        }
        while (cur_b != NULL) {
            BLOCK_WRAP* temp = cur_b;
            cur_b = cur_b->next;
            free(temp);
        }
        inode_hashtable[i] = NULL;
        block_hashtable[i] = NULL;
    }
}

// error: -1
// success: 0
// What is extend fail?
int extend(INODE_INFO* info, int newsize){

    /*
    int is_free(unsigned char* bitmap, int num, int len) {
        if(num < len)
            return !(bitmap[num / 8] & (1 << (num % 8)));
        return -1;
    }
    */
    int inode_blocks = (NUM_INODES  + (BLOCKSIZE / INODESIZE) - 1) / (BLOCKSIZE / INODESIZE);
    int valid_block = 0;
    // boot block + inode block
    for(int i = inode_blocks + 1; i < NUM_BLOCKS; i++){
        if(is_free(block_bitmap, i, NUM_BLOCKS) == 1){
            valid_block++;
        }
    }

    struct inode* inode = info->val;

    if(newsize < inode->size){
        TracePrintf(1, "in extend(): newsize < inode->size\n");
        return -1;
    }
    info->isDirty = 1;

    // count size, no need to -1
    // cur_block_max: if current size is 600 , block size is 512, then cur_block_max = 1024
    int cur_block_max = ((inode->size + (BLOCKSIZE-1)) / BLOCKSIZE) * BLOCKSIZE;
    int extra_block = 0;
    int used_block = (inode->size + (BLOCKSIZE-1)) / BLOCKSIZE;
    int new_block = (newsize + (BLOCKSIZE-1)) / BLOCKSIZE;

    if (used_block <= NUM_DIRECT && new_block > NUM_DIRECT && cur_block_max < BLOCKSIZE * NUM_DIRECT) {
        extra_block = 1; 
    }
    int need_block = new_block - used_block + extra_block;
    if(need_block > valid_block){
        TracePrintf(1, "in extend(): not enough free blocks\n");
        return -1;
    }

    // assign direct blocks
    if(cur_block_max < BLOCKSIZE * NUM_DIRECT){
        while(cur_block_max < BLOCKSIZE * NUM_DIRECT && cur_block_max < newsize){
	        int free_block = find_free(block_bitmap, NUM_BLOCKS);
	        if(free_block == -1) {
                TracePrintf(1, "in extend(): no free blocks\n");
		        return -1;
	        }
            set_bitmap_used(block_bitmap, free_block, NUM_BLOCKS);
	        BLOCK_INFO* new_block = get_block(free_block);
            new_block->isDirty = 1;
	        memset(new_block->data, '\0', BLOCKSIZE);
            // 0-indexed
	        inode->direct[cur_block_max / BLOCKSIZE] = free_block;
	        cur_block_max += BLOCKSIZE;
        }
    }
    
    // allocate indirect block
    // and ensure allocate only one time
    if(cur_block_max < newsize && cur_block_max == BLOCKSIZE * NUM_DIRECT){
        int indirect = find_free(block_bitmap, NUM_BLOCKS);
	    if(indirect == -1){
	        TracePrintf(1, "in extend(): no free blocks to assign indirect\n");
		    return -1;
        }
        set_bitmap_used(block_bitmap, indirect, NUM_BLOCKS);
	    inode->indirect = indirect;
    }

    // then allocate new indirect block
    if(cur_block_max < newsize){
	    BLOCK_INFO* block_indirect = get_block(inode->indirect);
	    block_indirect->isDirty = 1;
	    int* content = (int*)(block_indirect->data);
        // BLOCKSIZE / sizeof(int) how many blocks in a indirect block
        while(cur_block_max < (int)(BLOCKSIZE * (NUM_DIRECT + BLOCKSIZE / sizeof(int))) && cur_block_max < newsize ) {
	        int free_block = find_free(block_bitmap, NUM_BLOCKS);
            if(free_block == -1) {
                TracePrintf(1, "in extend(): no free blocks for indirect block\n");
		        return -1;
	        }
	        content[cur_block_max / BLOCKSIZE - NUM_DIRECT] = free_block;
            set_bitmap_used(block_bitmap, free_block, NUM_BLOCKS);
            cur_block_max += BLOCKSIZE;
	    }
    }
    inode->size = newsize;
    return 0;
}


int free_inode(int inodeNum){
    INODE_INFO* info = get_use_inode(inodeNum);
    if(info == NULL){
        TracePrintf(1, "in free_inode(): cannot find inode\n");
        return -1;
    }
    int j = 0;
	// direct blocks
	while(j < NUM_DIRECT && j * BLOCKSIZE < info->val->size){
        if(info->val->direct[j] > NUM_BLOCKS_FOR_INODES && info->val->direct[j] < NUM_BLOCKS){
            set_bitmap_free(block_bitmap,info->val->direct[j],NUM_BLOCKS);
        }
        else{
            TracePrintf(1, "in free_inode(): direct block num is in inode block\n");
            return -1;
        }
		j++;		    
	}
    memset(info->val->direct, 0, sizeof(info->val->direct));
	if(j * BLOCKSIZE < info->val->size){
		int* indirect_block = (int*)(get_block(info->val->indirect)->data);
		while(j * BLOCKSIZE < info->val->size){
            int indirect_blockNum = indirect_block[j - NUM_DIRECT];
            if(indirect_blockNum > NUM_BLOCKS_FOR_INODES && indirect_blockNum < NUM_BLOCKS){
                set_bitmap_free(block_bitmap,indirect_blockNum,NUM_BLOCKS);
            }
            else{
                TracePrintf(1, "in free_inode(): indirect block num is in inode block\n");
                return -1;
            }
		    j++;
		}
        set_bitmap_free(block_bitmap,info->val->indirect,NUM_BLOCKS);
	}

    info->val->size = 0;
    info->val->type = INODE_FREE;
    info->val->nlink = 0;
    info->isDirty = 1;
    info->val->indirect = 0;
    set_bitmap_free(inode_bitmap,info->inodeNum,NUM_INODES);
    sync();
    delete_inode_hashTable(info->inodeNum);
    remove_queue_inode(info);
    free(info->val);
    free(info);
    info = NULL;
    return 0;
}

int sync() {
    // This request writes all dirty cached inodes back to their corresponding disk blocks (in the cache) 
    // and then writes all dirty cached disk blocks to the disk. 
    // The request does not complete until all dirty inodes and disk blocks have been written to the disk; 
    // this request always then returns the value 0.
    INODE_INFO* cur_inode = inode_head;
    BLOCK_INFO* cur_block = block_head;
    while(cur_inode != inode_tail) {
        int inodeNum = cur_inode->inodeNum;
        if(cur_inode->isDirty == 1) {
            int blockNum = INODE_TO_BLOCK(inodeNum);
            int offset = INODE_IN_BLOCK_ADDR(inodeNum);
            BLOCK_INFO* tmp_block  = get_block(blockNum);
            /*
            if(tmp_block == NULL) {
                char data[BLOCKSIZE];
                ReadSector(blockNum, (void*)(data));
                memcpy(data + offset, cur_inode->val, sizeof(struct inode));
                int sig = WriteSector(blockNum, (void*)(data));
                if(sig == 0) {
         	        printf("in sync()...WriteSector Error\n");
                }
            }else{
                memcpy((void*)(tmp_block->data + offset), cur_inode->val, sizeof(struct inode));
                tmp_block->isDirty = 1;
            }
            */
            memcpy(tmp_block->data + offset, cur_inode->val, sizeof(struct inode));
            tmp_block->isDirty = 1;
            cur_inode->isDirty = 0;
        }
        cur_inode = cur_inode->next;
    }
  
    while(cur_block != block_tail) {
        if(cur_block->isDirty == 1) {
            int sig = WriteSector(cur_block->blockNum, (void*)(cur_block->data));
            if(sig == 0) {
                cur_block->isDirty = 0;
            }else{
                printf("in sync()...WriteSector Error\n");
                return -1;
            } 
            cur_block->isDirty = 0;
        }
        cur_block = cur_block->next;
    }
    return 0;
}

void print_bitmap(unsigned char* bitmap, int len_bits) {
    for (int i = 0; i < len_bits; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        int bit = (bitmap[byte_idx] >> bit_idx) & 1;
        printf("%d", bit);
        if ((i + 1) % 8 == 0) printf(" "); 
    }
    printf("\n");
}