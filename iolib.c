// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>
#include "iolib.h"
#include "yfs.h"

struct proc_cur_dir {
    int                 pid;
    short               inum;
    struct proc_cur_dir *next;
};

struct file {
    short   inum;
    int     position;
};

/*
    release all proc_cur_dir memory
    should be call in shutdown()
*/
void free_pcd(struct proc_cur_dir *cur) {
    if (cur == NULL) 
        return;
    if (cur->next != NULL)
        free_pcd(cur->next);
    free(cur);
}

struct file open_files[MAX_OPEN_FILES];
int open_files_count;
int open_files_flag = -1;
struct proc_cur_dir *pcd_head = NULL;



void Init_OpenfileVector(){
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        open_files[i].inum = -1;
        open_files[i].position = 0;
    }
    open_files_count = 0;
    open_files_flag = 0;
}
struct proc_cur_dir* find_cur_dir (int pid) {
    struct proc_cur_dir *cur = pcd_head;
    while (cur != NULL) {
        if (cur->pid == pid){
            return cur;
        }
        else{
            cur = cur->next;
        }
    }
    struct proc_cur_dir *newbie = malloc(sizeof(struct proc_cur_dir));
    newbie->pid = pid;
    newbie->inum = ROOTINODE;
    newbie->next = pcd_head;
    pcd_head = newbie;
    return newbie;
}

/* 
    create message buffer
    send it to ysf process
*/



int Open(char *pathname) {


    // pathname, pathname.size

    // link fd to inode
    // return fd
}

int Close(int fd) {

    // return ERROR if it isn't  a valid, exist fd

}

int Create(char *pathname) {


    // pathname, pathname.size

    // link fd to inode
    // return fd

}

int Read(int fd, void *buf, int size) {
    // used fd find index node, current position

    // inode, current position, buf, size

    // update current position
    
}

int Write(int fd, void *buf, int size) {
    // used fd find index node, current position

    // inode, current position, buf, size

    // update current position
}

int SymLink(char *oldname, char *newname) {
    // oldname, oldsize, newname, newsize
}

int ReadLink(char *pathname, char *buf, int len) {
    // pathname, pathname.size, buf, len
}


int MkDir(char *pathname) {
    // pathname, pathname.size
}

int RmDir(char *pathname) {
    // rm: "." and ".." may not be removed
    // pathname, pathname.size

}

int ChDir(char *pathname) {
    // pathname, pathname.size

}

int Stat(char *pathname, struct Stat *statbuf) {
    // pathname, pathname.size, statbuf
}

int Sync(void) {

}

int Shutdown(void) {

}

int Seek(int fd, int offset, int whence){

}
int Link(char *oldname, char *newname){

}
int Unlink(char *pathname){

}