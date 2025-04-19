// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>
#include "iolib.h"
#include "yfs.h"

short cur_dir_idx = ROOTINODE;
/* 
    create message buffer
    send it to ysf process
*/

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
    // oldname, oldsize, newname, newsize, current directory
}

int ReadLink(char *pathname, char *buf, int len) {
    // pathname, pathname.size, buf, len, current directory
}


int MkDir(char *pathname) {
    // pathname, pathname.size, current directory
}

int RmDir(char *pathname) {
    // rm: "." and ".." may not be removed
    // pathname, pathname.size

}

int ChDir(char *pathname) {
    // pathname, pathname.size, current directory

}

int Stat(char *pathname, struct Stat *statbuf) {
    // pathname, pathname.size, statbuf, current directory
}

int Sync(void) {

}

int Shutdown(void) {

}