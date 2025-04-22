#ifndef _LRU_H
#define _LRU_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include "block.h"



/*
#define	INODESIZE	64	// The size in bytes of each inode 
#define NUM_DIRECT	12	// number of direct block #s in inode 
*/

extern INODE_INFO* inode_head;  
extern INODE_INFO* inode_tail;  
extern BLOCK_INFO* block_head;  
extern BLOCK_INFO* block_tail;  
extern int inode_cache_len;  
extern int block_cache_len;    

void init_lru();
void dequeue_inode();
void enqueue_inode(INODE_INFO* x);
void remove_queue_inode(INODE_INFO* x);

void dequeue_block();
void enqueue_block(BLOCK_INFO* x);
void remove_queue_block(BLOCK_INFO* x);
void clear_block_cache();
void clear_inode_cache();


BLOCK_INFO* get_block_lru(int);
void set_block_lru(int , BLOCK_INFO*);
   
BLOCK_INFO* get_inode_lru(int);
void set_block_lru(int , BLOCK_INFO*);
void evict_block();
void evict_inode();




#endif 