#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
// #include "iolib.h"
#include "yfs.h"
#include <stdlib.h>
#include <string.h>


struct file {
    short   inum;
    int     position;
};


struct file open_files[MAX_OPEN_FILES];
int         open_files_count = 0;
int         open_files_flag = -1;
short       proc_cur_dir = ROOTINODE;



void InitOpenfileVector(){
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        open_files[i].inum = -1;
        open_files[i].position = 0;
    }
    open_files_count = 0;
    open_files_flag = 0;
}

/* 
    create message buffer
    send it to ysf process
*/



int Open(char *pathname) {
    TracePrintf(1, "Open(): start pathname, %s\n", pathname);
    if (open_files_flag == -1) { InitOpenfileVector(); }

    if (open_files_count == MAX_OPEN_FILES) {
        TracePrintf(1, "Open(): Reach MAX_OPEN_FILES!\n");
        return ERROR;
    }
    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    
    /*    
        message content
        1. CALL_OPEN                             1
        2. the address of pathname to be craeted    8
        3. the size of pathname                     4
        4. the inum of process current directory    2
    */
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }
    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_OPEN;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "Open(): Send error!\n");
        return ERROR;
    }

    int inum = *(int*)message;
    if (inum == -1) {
        TracePrintf(1, "Open(): File System create error!\n");
        return ERROR;
    }
    
    // link fd to inode
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].inum == -1) {
            open_files[i].inum = inum;
            open_files_count++;
            return i; // fd
        }
    }
    return ERROR;
}

int Close(int fd) {
    TracePrintf(1, "Close(): start fd %d\n", fd);
    if (open_files_flag == -1) { 
        InitOpenfileVector();
        return 0; 
    }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        TracePrintf(1, "Close(): Invalid file descriptor %d\n", fd);
        return ERROR;
    }

    if (open_files[fd].inum == -1) {
        TracePrintf(1, "Close(): Non-exit file for the file descriptor %d\n", fd);
        return ERROR;
    }
    
    open_files[fd].inum = -1;
    open_files[fd].position = 0;
    open_files_count--;
    return 0;


}

int Create(char *pathname) {
    TracePrintf(1, "Create(): start pathname, %s\n", pathname);
    if (open_files_flag == -1) { InitOpenfileVector(); }

    if (open_files_count == MAX_OPEN_FILES) {
        TracePrintf(1, "Create(): Reach MAX_OPEN_FILES!\n");
        return ERROR;
    }
    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    /*
        "aa/bb/cc"
        in yfs
            // use Copyfrom() and 2., 3. to get the pathname
            // 1. check "aa/bb" exist
            //     if not return ERROR
            // 2. check "cc" is a directory
            //     if yes return ERROR

    */

    /*    
        message content
        1. CALL_CREATE                              1
        2. the address of pathname to be craeted    8
        3. the size of pathname                     4
        4. the inum of process current directory    2
    */

    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 1); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    pathname_copy[pathname_size] = '\0';

    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_CREATE;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "Create(): Send error!\n");
        return ERROR;
    }

    int inum = *(int*)message;
    if (inum == -1) {
        TracePrintf(1, "Create(): File System create error!\n");
        return ERROR;
    }
    
    // link fd to inode
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].inum == -1) {
            open_files[i].inum = inum;
            open_files_count++;
            return i; // fd
        }
    }
    return ERROR;

}

int Read(int fd, void *buf, int size) {
    TracePrintf(1, "Read(): start fd %d\n", fd);
    if (open_files_flag == -1) { InitOpenfileVector(); }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        TracePrintf(1, "Read(): Invalid file descriptor %d\n", fd);
        return ERROR;
    }

    if (open_files[fd].inum == -1) {
        TracePrintf(1, "Read(): Non-exit file for the file descriptor %d\n", fd);
        return ERROR;
    }


    // inode, current position, buf, size
    /*
        message content
        1. CALL_READ                1
        2. the inum of fd           2
        3. the current position     4
        4. the address of buf       8
        5. the size of read len     4
    */
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_READ;
    memcpy(message + 1, &open_files[fd].inum, sizeof(short));
    memcpy(message + 3, &open_files[fd].position, sizeof(int));
    memcpy(message + 7, &buf, sizeof(void*));
    memcpy(message + 15, &size, sizeof(int));
    if (Send(message, -FILE_SERVER) == -1) {
        TracePrintf(1, "Read(): Send error!\n");
        return ERROR;
    }
    
    // update current position
    int offset = *(int*)message;
    open_files[fd].position += offset;
    return offset;
}

int Write(int fd, void *buf, int size) {
    TracePrintf(1, "Write(): start fd %d\n", fd);
    if (open_files_flag == -1) { InitOpenfileVector(); }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        TracePrintf(1, "Write(): Invalid file descriptor %d\n", fd);
        return ERROR;
    }

    if (open_files[fd].inum == -1) {
        TracePrintf(1, "Write(): Non-exit file for the file descriptor %d\n", fd);
        return ERROR;
    }

    // inode, current position, buf, size
    /*
        message content
        1. CALL_WRITE               1
        2. the inum of fd           2
        3. the current position     4
        4. the address of buf       8
        5. the size of write len    4
    */
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_WRITE;
    memcpy(message + 1, &open_files[fd].inum, sizeof(short));
    memcpy(message + 3, &open_files[fd].position, sizeof(int));
    memcpy(message + 7, &buf, sizeof(void*));
    memcpy(message + 15, &size, sizeof(int));
    if (Send(message, -FILE_SERVER) == -1) {
        TracePrintf(1, "Write(): Send error!\n");
        return ERROR;
    }
    
    // update current position
    int offset = *(int*)message;
    open_files[fd].position += offset;
    return offset;
}

int SymLink(char *oldname, char *newname) {
    TracePrintf(1, "SymLink(): start old %s, new %s\n", oldname, newname);
    // oldname, oldsize, newname, newsize
    /*
        message content
        1. CALL_SYMLINK             1                  
        2. the address of oldname   8           
        3. the size of oldname      4                   
        4. the address of newname   8                
        5. the size of newname      4      
        6. the inum of cur dir      2
    */

    size_t len1 = strlen(oldname) + 1;  
    if (len1 > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: oldname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len1);
        return ERROR;
    }

    size_t len2 = strlen(newname) + 1;  
    if (len2 > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: newname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len2);
        return ERROR;
    }

    int oldname_size = strlen(oldname);
    char* oldname_copy = (char*)malloc(oldname_size + 2); // add "\0"
    memcpy(oldname_copy, oldname, oldname_size);
    if (oldname[oldname_size-1] == '/') {
        oldname_copy[oldname_size] = '.';
        oldname_copy[oldname_size+1] = '\0';
    }
    else {
        oldname_copy[oldname_size] = '\0';
    }

    int newname_size = strlen(newname);
    char* newname_copy = (char*)malloc(newname_size + 2); // add "\0"
    memcpy(newname_copy, newname, newname_size);
    if (newname[oldname_size-1] == '/') {
        newname_copy[newname_size] = '.';
        newname_copy[newname_size+1] = '\0';
    }
    else {
        newname_copy[newname_size] = '\0';
    }

    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_SYMLINK;
    memcpy(message + 1, &oldname_copy, sizeof(char*));
    memcpy(message + 9, &oldname_size, sizeof(int));
    memcpy(message + 13, &newname_copy, sizeof(char*));
    memcpy(message + 21, &newname_size, sizeof(int));
    memcpy(message + 25, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(oldname_copy);
    free(newname_copy);
    if (res == -1) {
        TracePrintf(1, "Link(): Send error!\n");
        return ERROR;
    }
    int read_len = *(int*)message;
    if (read_len == -1) {
        TracePrintf(1, "Link(): Cannot link path %s to %s\n", oldname, newname);
        return ERROR;
    }
    return 0;
}

int ReadLink(char *pathname, char *buf, int len) {
    TracePrintf(1, "Readlink(): start pathname, %s\n", pathname);


    size_t len1 = strlen(pathname) + 1;  
    if (len1 > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len1);
        return ERROR;
    }


    // pathname, pathname.size, buf, len
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }
    /*
        message content
        1. CALL_READLINK            1
        2. the address of pathname  8 
        3. the size of pathname     4                
        3. the address of buf       8
        4. the size of write len    4  
        5. the inum of cur dir      2
    */
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_READLINK;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &buf, sizeof(char*));
    memcpy(message + 21, &len, sizeof(int));
    memcpy(message + 25, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "Readlink(): Send error!\n");
        return ERROR;
    }
    int read_len = *(int*)message;
    if (read_len == -1) {
        TracePrintf(1, "Readlink(): Cannot read link path %s\n", pathname);
        return ERROR;
    }
    return read_len;
}


int MkDir(char *pathname) {
    TracePrintf(1, "MkDir(): start pathname, %s\n", pathname);

    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    // pathname, pathname.size
    /*
        (o) mkdir tt    create tt
        (o) mkdir tt/   create tt
        (x) mkdir tt/.  create . in tt, but tt doesn't eixst
        message content
        1. CALL_MKDIR               1
        2. the address of pathname  8
        3. the size of pathname     4
        4. the inum of cur dir      2
    */
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 1); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname_size != 1 && pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size-1] = '\0';
    }
    pathname_copy[pathname_size] = '\0';
    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_MKDIR;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "MkDir(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    if (status == -1) {
        TracePrintf(1, "MkDir(): Cannot create directory %s\n", pathname);
        return ERROR;
    }
    return 0;
}

int RmDir(char *pathname) {
    TracePrintf(1, "RmDir(): start pathname, %s\n", pathname);
    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    // rm: "." and ".." may not be removed
    /*
        (x) rm -r tt/.
        (x) rm -r tt/..
        (o) rm -r tt/./ttt
        (o) rm -r tt/ttt/../ttt
        message content
        1. CALL_RMDIR               1
        2. the address of pathname  8
        3. the size of pathname     4
        4. the inum of cur dir      2
    */
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }
    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_RMDIR;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "RmDir(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    if (status == -1) {
        TracePrintf(1, "RmDir(): Cannot delete directory %s\n", pathname);
        return ERROR;
    }
    return 0;
}

int ChDir(char *pathname) {
    TracePrintf(1, "ChDir(): start pathname, %s\n", pathname);

    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    /*
        (o) cd tt
        (o) cd tt/
        (o) cd tt/.
        message content
        1. CALL_CHDIR               1
        2. the address of pathname  8
        3. the size of pathname     4
        4. the inum of cur dir      2
    */
    // handle "aa/bb/" -> "aa/bb/."
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }
    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_CHDIR;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "ChDir(): Send error!\n");
        return ERROR;
    }
    int inum = *(int*)message;
    if (inum == -1) {
        TracePrintf(1, "ChDir(): Cannot change to directory %s\n", pathname);
        return ERROR;
    }
    proc_cur_dir = inum;
    return 0;
}

int Stat(char *pathname, struct Stat *statbuf) {
    TracePrintf(1, "Stat(): start pathname, %s\n", pathname);
    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    /*
        message content
        1. CALL_STAT                1
        2. the address of pathname  8
        3. the size of pathname     4
        4. the address of statbuf   8
        5. the inum of cur dir      2
    */
    // do i need to handle "aa/bb/" -> "aa/bb/."?
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }

    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_STAT;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &statbuf, sizeof(struct Stat*));
    memcpy(message + 21, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "Stat(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    return status;

}

int Sync(void) {
    /*
        message content
        1. CALL_SYNC 1
    */
    char message[MESSAGE_SIZE];
    message[0] = (char) CALL_SYNC;
    if (Send(message, -FILE_SERVER) == -1) {
        TracePrintf(1, "Sync(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    return status;

}

int Shutdown(void) {
    /*
        message content
        1. CALL_SHUTDOWN 1
    */
    char message[MESSAGE_SIZE];
    message[0] = (char) CALL_SHUTDOWN;
    if (Send(message, -FILE_SERVER) == -1) {
        TracePrintf(1, "Sync(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    return status;
}

int Seek(int fd, int offset, int whence) {
    TracePrintf(1, "Seek(): start fd %d, offset %d, whence %d\n", fd, offset, whence);
    if (open_files_flag == -1) { InitOpenfileVector(); }

    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        TracePrintf(1, "Seek(): Invalid file descriptor %d\n", fd);
        return ERROR;
    }

    if (open_files[fd].inum == -1) {
        TracePrintf(1, "Seek(): Non-exit file for the file descriptor %d\n", fd);
        return ERROR;
    }

    int new_position = 0;
    switch(whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = open_files[fd].position + offset;
            break;
        case SEEK_END:
            /*
                message content
                1. CALL_SEEK   1
                2. inum of fd  2
                
            */
            char message[MESSAGE_SIZE];
            message[0] = (char)CALL_SEEK;
            memcpy(message + 1,(void*)(&open_files[fd].inum), sizeof(short));

            if (Send(message, -FILE_SERVER) == -1) {
                TracePrintf(1, "Seek(): Send error!\n");
                return ERROR;
            }
            int file_size = *(int*)message;
            new_position = file_size + offset;
            break;
        default:
            TracePrintf(1, "Seek(): Invalid whence value %d\n", whence);
            return ERROR;
    }

    if (new_position < 0) {
        TracePrintf(1, "Seek(): Invalid new position %d\n", new_position);
        return ERROR;
    }
    open_files[fd].position = new_position;
    return new_position;
}

int Link(char *oldname, char *newname) {
    TracePrintf(1, "Link(): start old %s, new %s\n", oldname, newname);
    size_t len1 = strlen(oldname) + 1;  
    if (len1 > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: oldname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len1);
        return ERROR;
    }

    size_t len2 = strlen(newname) + 1;  
    if (len2 > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: newname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len2);
        return ERROR;
    }
    /*
        message content
        1. CALL_LINK                1                  
        2. the address of oldname   8           
        3. the size of oldname      4                   
        4. the address of newname   8                
        5. the size of newname      4      
        6. the inum of cur dir      2
    */
    int oldname_size = strlen(oldname);
    char* oldname_copy = (char*)malloc(oldname_size + 2); // add "\0"
    memcpy(oldname_copy, oldname, oldname_size);
    if (oldname[oldname_size-1] == '/') {
        oldname_copy[oldname_size] = '.';
        oldname_copy[oldname_size+1] = '\0';
    }
    else {
        oldname_copy[oldname_size] = '\0';
    }
    

    int newname_size = strlen(newname);
    char* newname_copy = (char*)malloc(newname_size + 2); // add "\0"
    memcpy(newname_copy, newname, newname_size);
    if (newname[oldname_size-1] == '/') {
        newname_copy[newname_size] = '.';
        newname_copy[newname_size+1] = '\0';
    }
    else {
        newname_copy[newname_size] = '\0';
    }

    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_LINK;
    memcpy(message + 1, &oldname_copy, sizeof(char*));
    memcpy(message + 9, &oldname_size, sizeof(int));
    memcpy(message + 13, &newname_copy, sizeof(char*));
    memcpy(message + 21, &newname_size, sizeof(int));
    memcpy(message + 25, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(oldname_copy);
    free(newname_copy);
    if (res == -1) {
        TracePrintf(1, "Link(): Send error!\n");
        return ERROR;
    }
    int read_len = *(int*)message;
    if (read_len == -1) {
        TracePrintf(1, "Link(): Cannot link path %s to %s\n", oldname, newname);
        return ERROR;
    }
    return 0;
}

int Unlink(char *pathname) {
    TracePrintf(1, "Unlink(): start pathname, %s\n", pathname);
    size_t len = strlen(pathname) + 1;  
    if (len > MAXPATHNAMELEN) {
        TracePrintf(1,"Error: pathname exceeds MAXPATHNAMELEN (%d). Length is %zu.\n", MAXPATHNAMELEN, len);
        return ERROR;
    }
    /*
        message content
        1. CALL_UNLINK              1
        2. the address of pathname  8
        3. the size of pathname     4
        4. the inum of cur dir      2
    */
    int pathname_size = strlen(pathname);
    char* pathname_copy = (char*)malloc(pathname_size + 2); // add "\0"
    memcpy(pathname_copy, pathname, pathname_size);
    if (pathname[pathname_size-1] == '/') {
        pathname_copy[pathname_size] = '.';
        pathname_copy[pathname_size+1] = '\0';
    }
    else {
        pathname_copy[pathname_size] = '\0';
    }
    // fill in message
    char message[MESSAGE_SIZE];
    message[0] = (char)CALL_UNLINK;
    memcpy(message + 1, &pathname_copy, sizeof(char*));
    memcpy(message + 9, &pathname_size, sizeof(int));
    memcpy(message + 13, &proc_cur_dir, sizeof(short));
    int res = Send(message, -FILE_SERVER);
    free(pathname_copy);
    if (res == -1) {
        TracePrintf(1, "Unlink(): Send error!\n");
        return ERROR;
    }
    int status = *(int*)message;
    if (status == -1) {
        TracePrintf(1, "Unlink(): Cannot unlink path %s\n", pathname);
        return ERROR;
    }
    return 0;
}