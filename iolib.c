// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>
#include "iolib.h"
#include "yfs.h"

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