#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// #include <comp421/iolib.h>
// #include <comp421/filesystem.h>
// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
#include "block.h"
#include "lrucache.h"


struct inode_info* get_use_inode(int inodeNum){
    INODE_INFO* result  = get_inode_lru(inodeNum);
    if(result->inodeNum == -1){
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
    
    INODE_INFO* inode_header = get_inode(0);
    struct fs_header* myHeader = (struct fs_header*)(inode_header->val);

    // init hasg table to null
    memset(inode_hashtable, 0, sizeof(inode_hashtable));
    memset(block_hashtable, 0, sizeof(block_hashtable));
    
    
    NUM_INODES = myHeader->num_inodes;
    NUM_BLOCKS = myHeader->num_blocks;


    int block_bitmap_size = (NUM_BLOCKS + 7) / 8;
    int inode_bitmap_size = (NUM_INODES + 7) / 8;
    block_bitmap = (unsigned char*)malloc(block_bitmap_size);
    inode_bitmap = (unsigned char*)malloc(inode_bitmap_size);
    memset(block_bitmap, 0, block_bitmap_size);
    memset(inode_bitmap, 0, inode_bitmap_size);
    // boot block
    set_bitmap_used(block_bitmap,0,NUM_BLOCKS); //TODO
    // fs_header
    set_bitmap_used(inode_bitmap,0,NUM_INODES); //TODO


    int i;
    // set block list for inodes used
    for(i = 1; i < (NUM_INODES * INODESIZE) / BLOCKSIZE ; i++){
        set_bitmap_used(block_bitmap,i,NUM_BLOCKS);
    }
    // check all inodes
    for(i = 1; i < NUM_INODES; i++){
	    struct inode* temp = get_use_inode(i)->val;
	    if(temp->type == INODE_FREE){
            set_bitmap_used(inode_bitmap,i,NUM_INODES);
	        int j = 0;
	        // direct blocks
	        while(j < NUM_DIRECT && j * BLOCKSIZE < temp->size){
		        set_bitmap_used(block_bitmap,temp->direct[j],NUM_BLOCKS);
		        j++;		    
	        }

	        if(j * BLOCKSIZE < temp->size){
		        int* indirect_block = (int*)(get_block(temp->indirect)->data);
		        while(j * BLOCKSIZE < temp->size){
                    int indirect_blockNum = indirect_block[j - NUM_DIRECT];
		            set_bitmap_used(block_bitmap,indirect_blockNum,NUM_BLOCKS);
		            j++;
		        }
	        }
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

INODE_INFO* search_inode_hashTable(int key) {
    int index = hash_key(key);
    INODE_WRAP* current = inode_hashtable[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;  
        }
        current = current->next;
    }
    return NULL;  
}

BLOCK_INFO* search_block_hashTable(int key) {
    int index = hash_key(key);
    BLOCK_WRAP* current = block_hashtable[index];
    while (current != NULL) {
        if (current->key == key) {
            return current->value;  
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


int extend(struct inode_info* info, int newsize){
    struct inode* file_inode = info->inode_val;
    info->dirty = 1;
    if(newsize < file_inode->size){
	return 0;
    }

    info->dirty = 1;
    // round filesize up to the next blocksize
    int current = ((file_inode->size + (BLOCKSIZE-1)) / BLOCKSIZE) * BLOCKSIZE;
    // fill up direct blocks first
    if(current < BLOCKSIZE * NUM_DIRECT){
        while(current < BLOCKSIZE * NUM_DIRECT && current < newsize){
	    // assign a new block in direct
	    int free_block = get_free_block();
	    if(free_block == ERROR) {
		return ERROR;
	    }

	    struct block_info* info = read_block_from_disk(free_block);
	    info->dirty = 1;
	    memset(info->data, '\0', BLOCKSIZE);

	    file_inode->direct[current / BLOCKSIZE] = free_block;
	    current += BLOCKSIZE;
        }
    }
    
    // If this is the first time growing into indirect size then allocate indirect block
    if(current < newsize && current == BLOCKSIZE * NUM_DIRECT){
        int new_indirect = get_free_block();
	if(new_indirect == ERROR)
	    return ERROR;
	file_inode->indirect = new_indirect;
    }

    // if direct blocks not enough, then access indirect blocks
    if(current < newsize){
	int big_block_num = file_inode->indirect;
	struct block_info * block_indirect = read_block_from_disk(big_block_num);
	block_indirect->dirty = 1;
	int * int_array = (int*)(block_indirect->data);
	
        while(current < BLOCKSIZE * (NUM_DIRECT + BLOCKSIZE / sizeof(int)) && current < newsize ) {
	    int free_block = get_free_block();
	    if(free_block == ERROR) {
		return ERROR;
	    }
	    int_array[current / BLOCKSIZE - NUM_DIRECT] = free_block;
	    current += BLOCKSIZE;
	}
    }
    file_inode->size = newsize;
    return 0;
}