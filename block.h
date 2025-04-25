#ifndef _BLOCK_H
#define _BLOCK_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

// #include "yalnix.h"
// #include "hardware.h"
// #include "filesystem.h"
// #include "iolib.h"

#define INODE_TO_BLOCK(inodeNum) (1 + ((inodeNum) / (BLOCKSIZE / INODESIZE)))
#define INODE_IN_BLOCK_ADDR(inodeNum) (((inodeNum) % (BLOCKSIZE / INODESIZE)) * INODESIZE)

#define HASH_TABLE_SIZE 256



void set_bitmap_used(unsigned char* , int, int);
void set_bitmap_free(unsigned char* , int, int);
int is_free(unsigned char* , int, int);
int find_free(unsigned char* , int);

int hash_key(int);


typedef struct inode_info{
    int isDirty;
    int inodeNum;
    struct inode_info *next;
    struct inode_info *prev;
    struct inode *val; 
} INODE_INFO;

// used for hash hable
typedef struct inode_wrap{
    int key;
    struct inode_info* value;
    struct inode_wrap* next;
} INODE_WRAP;

typedef struct block_info{
    int isDirty;
    int blockNum;
    struct block_info* prev;
    struct block_info* next;
    // BLOCKSIZE = SECTORSIZE = 512
    char data[BLOCKSIZE];
} BLOCK_INFO;

typedef struct block_wrap{
    int key;
    struct block_info* value;
    struct block_wrap* next;
}BLOCK_WRAP;


// int NUM_BLOCKS;
// int NUM_INODES;
// int NUM_BLOCKS_FOR_INODES;
// unsigned char* block_bitmap;
// unsigned char* inode_bitmap;
// INODE_WRAP* inode_hashtable[HASH_TABLE_SIZE]; 
// BLOCK_WRAP* block_hashtable[HASH_TABLE_SIZE]; 

extern int NUM_BLOCKS;
extern int NUM_INODES;
extern int NUM_BLOCKS_FOR_INODES;
extern unsigned char* block_bitmap;
extern unsigned char* inode_bitmap;
extern INODE_WRAP* inode_hashtable[HASH_TABLE_SIZE]; 
extern BLOCK_WRAP* block_hashtable[HASH_TABLE_SIZE]; 

// init hashTable and bitMap
void init_inode_block();
// get inode from disk
struct inode_info* get_use_inode(int);
struct inode_info* get_new_inode();
struct block_info* get_block(int);

void insert_inode_hashTable(int , INODE_INFO* );
void insert_block_hashTable(int , BLOCK_INFO* );
INODE_WRAP* search_inode_hashTable(int);
BLOCK_WRAP* search_block_hashTable(int);
void delete_inode_hashTable(int);
void delete_block_hashTable(int);
void clear_inode_block_hashTable();
int extend(INODE_INFO* , int);

int free_inode(int);
int sync();
void print_bitmap(unsigned char* , int);

#endif 