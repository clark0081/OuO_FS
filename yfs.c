#include "yfs.h"

int MessageHandler(char *msg, int pid)
{
    return 0;
}

int SymLinkHandler(char *oldname, char *newname, short cur_dir_idx)
{   
    /*
        e.g  "00/11/22/33/44"
        - get the file name 
            - "44"
        - get the parent directory
            - "0/11/22/33/"
        - create a new file under parent_dir
        - store(wtite) the oldname in newname's file // %$%
    */
    return 0;
}

int ReadLinkHandler(char *pathname, char *buf, int len, short cur_dir_idx) 
{
    /*
        get the index node of pathname, whose type should be INODE_SYMLINK
            for d_e in parent_dir:
                read dir_entry from parent_dir's file // %$%
                if not symlink:
                    continue
                if d_e.name ==  filename
                    get the inode
        if not exist:
            return ERROR
        read the oldname from symlink's file // %$%
        
        
    */
    return 0;
}

int MkDirHandler(char *pathname, short cur_dir_idx)
{
    /*
        - create a new file under parent_dir
        - add dir_entries of ".", ".." to the new index node
    */
    return 0;
}

int RmDirHandler(char *pathname, short cur_dir_idx)
{
    /*
        - check removable path name
            not root
            not "."
            not ".."
        - get the index node
            handle error
        - check whether there are files in this inode
            if there is any  --> return ERROR
        - get its parent index node
        - remove this file
            clean parent
            free this file
    */
    return 0;
}

int ChDirHandler(char *pathname, short cur_dir_idx)
{
    /*
        - get the index node
            handle error
        - return node's index
    */
    return 0;
}

int StatHandler(char *pathname, struct Stat *statbuf, short cur_dir_idx)
{
    /*
        - get the index node
            handle error
        - fill index, type, size, nlink to statbuf
    */
    return 0;
}

int SyncHandler()
{   
    /* write back all inode/block cache */
    return 0;
}

int ShutdownHandler()
{
    // SyncHandler()

    return 0;
}

int main(int argc, char** argv) {
    int pid = Fork();
    if (pid == 0) {
        // child process
        Exec(argv[1], argv + 1);
    }
    else {
        while(1) {
            char msg[MESSAGE_SIZE];
            int pid = Receive(msg);
            if (pid == 0) {
                // deadlock
                TracePrintf(0, "Receive() return 0 to avoid deadlock");
                Exit(0);
            }
            else if (pid == -1) {
                // error
                TracePrintf(0, "Receive() return -1 due to error");
                Exit(0);
            }
            int res = MessageHandler(msg, pid); // pid for using CopyFrom()/ CopyTo()

            for (int i = 0; i < MESSAGE_SIZE; i++) { msg[i] = '\0'; }  // clean the msg
            memcpy(msg, &res, sizeof(int));
            Reply(msg, pid);
        }
    }
    return 0;
}
