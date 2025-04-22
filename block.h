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

#define INODE_TO_BLOCK(inodeNum) (1 + ((inodeNum) / (BLOCKSIZE / INODESIZE)))
#define INODE_IN_BLOCK_ADDR(inodeNum) (((inodeNum) % (BLOCKSIZE / INODESIZE)) * INODESIZE)

#define HASH_TABLE_SIZE 256
/*
struct inode {
    short type;			// file type (e.g., directory or regular) 
    short nlink;		// number of hard links to inode 
    int   reuse;		// inode reuse count 
    int   size;			// file size in bytes 
    int   direct[NUM_DIRECT];	// block numbers for 1st NUM_DIRECT blocks 
    int   indirect;		// block number of indirect block 
};
*/

// all run in yfs

int NUM_BLOCKS;
int NUM_INODES;
unsigned char* block_bitmap = NULL;
unsigned char* inode_bitmap = NULL;




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
    struct block* prev;
    struct block* next;
    // BLOCKSIZE = SECTORSIZE = 512
    char data[BLOCKSIZE];
} BLOCK_INFO;

typedef struct block_wrap{
    int key;
    struct block_info* value;
    struct block_wrap* next;
}BLOCK_WRAP;


INODE_WRAP* inode_hashtable[HASH_TABLE_SIZE]; 
BLOCK_WRAP* block_hashtable[HASH_TABLE_SIZE]; 


void init_inode_block();
// get inode from disk
struct* inode_info get_inode(int);
struct* block_info get_block(int);
void insert_inode_hashTable(int , INODE_INFO* );
void insert_block_hashTable(int , BLOCK_INFO* );
INODE_INFO* search_inode_hashTable(int);
BLOCK_INFO* search_block_hashTable(int);
void delete_inode_hashTable(int);
void delete_block_hashTable(int);
void free_inode_block_hashTable();
//-------------------------------------------------------------
//LRU




//-------------------------------------------------------------


/*
struct block_info* block_front;
struct block_info* block_rear;

struct block_wrap* block_hashtable[BLOCK_CACHESIZE]; 
struct block_wrap* default_block_wrap;
struct block_info* default_block_info;

//Inode variable
int current_inodecache_number;



struct inode_info* inode_front;
struct inode_info* inode_rear;

struct inode_wrap* inode_hashtable[INODE_CACHESIZE]; 
struct inode_wrap* default_inode_wrap;
struct inode_info* default_inode_info;



void init();


struct block_wrap *get_block(int key);
void put_block_to_hashtable(int key, struct block_info* data_input);
struct block_wrap* remove_block_from_hashtable(int block_num);
void enqueue_block(struct block_info * x);
void dequeue_block();
void remove_queue_block(struct block_info * x);
struct block_info* get_lru_block(int block_num);
int sync();
void evict_block(); 
void set_lru_block(int block_num, struct block_info* input_block);
struct inode_wrap *get_inode(int key);

void enqueue_inode(struct inode_info * x);
void dequeue_inode();
void remove_queue_inode(struct inode_info * x);
struct inode_info* get_lru_inode(int inode_num);
void set_lru_inode(int inode_num, struct inode_info* input_inode);



int FSOpen(char *pathname, short current_dir);
int FSCreate(char *pathname, short current_dir);
int FSRead(void *buf, int size, short inode, int position);
int FSWrite(void *buf, int size, short inode, int position);
int FSSeek(short inode);
int FSLink(char *oldname, char *newname, short current_dir);
int FSUnlink(char *pathname, short current_dir);
int FSSymLink(char *oldname, char *newname, short current_dir);
int FSReadLink(char *pathname, char *buf, int len, short current_dir);
int FSMkDir(char *pathname, short current_dir);
int FSRmDir(char *pathname, short current_dir);
int FSChDir(char *pathname, short current_dir);
int FSStat(char *pathname, struct Stat* statbuf, short current_dir);
int FSSync(void);
int FSShutdown(void);
int Redirect_Call(char* msg, int pid);

void set_lru_inode(int inode_num, struct inode_info* input_inode);
*/


#endif 