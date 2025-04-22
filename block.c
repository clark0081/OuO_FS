#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include "block.h"


struct inode_info* get_inode(int inodeNum){
	/*
    struct inode_info* result = get_lru_inode(inode_num);
	if(result->inode_number == -1) {
		int block_num = calculate_inode_to_block_number(inode_num);
		struct block_info* tmp = get_lru_block(block_num);
		if(tmp->block_number == -1) {
			tmp = read_block_from_disk(block_num);

		}else{
			//The block is in the cache.

		}
	}else{
		return result;
	}
    */
    INODE_INFO* res = (INODE_INFO*)malloc(sizeof(INODE_INFO));
    int blockNum = INODE_TO_BLOCK(inodeNum);
    BLOCK_INFO* tempBlock = get_block(blockNum);
    int offset = INODE_IN_BLOCK_ADDR(inodeNum);
    struct inode* myInode = (struct inode*)(tempBlock->data + offset);
    res->next = NULL;
    res->prev = NULL;
    res->val = myInode;
    return res;
}

BLOCK_INFO* get_block(int blockNum) {
    // check LRU
	//struct block_info* result = get_lru_block(block_num);
    /*
	if(result->block_number == -1) {
		//Reads from the disk.
		result = (struct block_info*)malloc(sizeof(struct block_info));
		ReadSector(block_num, (void*)(result->data));
		//Sets the dirty to not dirty
		result->dirty = 0;
		set_lru_block(block_num, result);
		//To obtain the block_info.
		return get_lru_block(block_num);

	}else{
		return result;
	}
    */
   BLOCK_INFO* res = (BLOCK_INFO*)malloc(sizeof(BLOCK_INFO));
   ReadSector(blockNum, (void*)(res->data));
   res->isDirty = 0;
   return res;
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
    set_bitmap_used(block_bitmap,0);
    // fs_header
    set_bitmap_used(inode_bitmap,0);


    int i;
    // set block list for inodes used
    for(i = 1; i < (NUM_INODES * INODESIZE) / BLOCKSIZE ; i++){
        set_bitmap_used(block_bitmap,i,block_bitmap_size);
    }
    // check all inodes
    for(i = 1; i < NUM_INODES; i++){
	    struct inode* temp = get_inode(i)->val;
	    if(temp->type == INODE_FREE){
            set_bitmap_used(inode_bitmap,i,inode_bitmap_size);
	        int j = 0;
	        // direct blocks
	        while(j < NUM_DIRECT && j * BLOCKSIZE < temp->size){
		        set_bitmap_used(block_bitmap,temp->direct[j],block_bitmap_size);
		        j++;		    
	        }

	        if(j * BLOCKSIZE < temp->size){
		        int* indirect_block = (int*)(get_block(temp->indirect)->data);
		        while(j * BLOCKSIZE < temp->size){
                    int indirect_blockNum = indirect_block[j - NUM_DIRECT];
		            set_bitmap_used(block_bitmap,indirect_blockNum,block_bitmap_size);
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
void free_inode_block_hashTable() {
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