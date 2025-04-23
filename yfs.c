#include "yfs.h"
#include "block.h"
#include "lrucache.h"


short findInumInDir(char * filename, short dir) {
    struct inode_info* dir_inode = get_inode(dir);
    // TODO: handle invalud dir_inode

    
    // int numEntries = info->inodeVal->size / sizeof(struct dir_entry);
    int n = dir_inode->val->size / sizeof(struct dir_entry);
    int i;
    struct dir_entry entry;

    for (i = 0; i < n; i++) {
        if (ReadHandler(dir, i * sizeof(struct dir_entry), (void*)&entry, sizeof(struct dir_entry)) == -1) {
            TracePrintf(1, "findInumInDir() cannot read dir entry in directory %d\n", dir);
            return ERROR;
        }

        if (strncmp(filename, entry.name, DIRNAMELEN) == 0) {
            // INODE_INFO* next_inode_info = get_inode(entry.inum);
            // if (next_inode_info->val->type != INODE_DIRECTORY && next_inode_info->val->type != INODE_SYMLINK) {
            //     continue;
            // }
            TracePrintf(1, "findInumInDir() find %s with inum %d in directory %d\n", filename, entry.inum, dir);
            return entry.inum;
        }
    }
    return 0;
}

short resolvePath(char *pathname, short cur_dir_in) {
    // check absolute path or not
    char *org_pathname = pathname; // for Traceprintf
    short cur_dir = (pathname[0]=='/') ? ROOTINODE : cur_dir_in;

    // symlink count
    int symlink_count = 0;
    char *alloc_pathname = NULL;  

    //
    char node_name[DIRNAMELEN + 1];
    memset(node_name,'\0',DIRNAMELEN + 1);

    while(pathname[0] == '/') {
        pathname++;
    }

    // main loop
    while (strlen(pathname) > 0) {
        // initialize
        int path_len = strlen(pathname);
		memset(node_name,'\0',DIRNAMELEN + 1);
        
        /*
            /bbb/ccc
            |
        */

        while(path_len > 0 && pathname[0] == '/') {
            pathname++;
            path_len--;
        }

        /*
            /bbb/ccc
             |
        */

        // copy "bb" to node_name
        int i = 0;
        while(path_len > 0 && pathname[0] != '/') {
            node_name[i++] = *pathname;
			pathname++;
			path_len--;
		}

        /*
            /bbb/ccc
                |
        */

        short inum = findInumInDir(node_name, cur_dir); // the entry I just found
        if (inum == -1) {
            TracePrintf(1, "resolvePath() cannot find files %s in directory %d for %s\n", node_name, cur_dir, org_pathname);
            return ERROR;
        }
        INODE_INFO* next_inode_info = get_inode(inum);

        if (next_inode_info->val->type == INODE_SYMLINK) {
            if(symlink_count >= MAXSYMLINKS) {
                TracePrintf(1, "resolvePath() reach maximun symlink in traversal for %s.\n", org_pathname);
                return ERROR;
            }
            else {
                BLOCK_INFO *first_block = get_block(next_inode_info->val->direct[0]);

                int new_path_len = next_inode_info->val->size + path_len + 1;
                char *new_pathname = (char*)malloc(new_path_len);
                memset(new_pathname, 0, new_path_len); 

                // copy content
                memcpy(new_pathname, first_block->data, next_inode_info->val->size);
                if (path_len > 0) {
                    strcat(new_pathname, "/");
					strcat(new_pathname, pathname);
                }

                // free alloc pathname
                if (alloc_pathname) { free(alloc_pathname); }
                alloc_pathname = new_pathname;

                pathname = new_pathname;

                symlink_count++;
                if (new_pathname[0] == '/') { 
                    cur_dir = ROOTINODE; 
                    continue;
                }
            }
        }
        cur_dir = inum;
    }
       
    if (alloc_pathname) { free(alloc_pathname); }
    
    return cur_dir;

}

int getParentInum(char *pathname, short cur_dir_in) {
    // find the last "/"
    int parent_len = 0;
    for (int i = strlen(pathname) - 1; i >= 0; i--) {
        if (pathname[i] == '/') {
            parent_len = i+1;
            break;
        }
    }
    /*
        aa/bb/cc
             |
    */

    // root
    if (parent_len == 1 && pathname[0] == '/') {
        return ROOTINODE;
    }

    // non root
    /*
        aa/bb/cc            aa/bb/cc
             |                  |
        p_len = 6           p_len = 5
        
        we don't want to copy the last slash
    */
    parent_len -= 1;
    char* parent_pathname_copy = (char*)malloc(parent_len + 1); // add "\0"
    memcpy(parent_pathname_copy, pathname, parent_len);
    parent_pathname_copy[parent_len] = '\0';

    short parent_inum = resolvePath(parent_pathname_copy, cur_dir_in);
    free(parent_pathname_copy);

    if(parent_inum == -1){
        TracePrintf(1, "getParentInum() cannot get parent inum for %s\n",pathname);
        return ERROR;
    }
    return parent_inum;



}
char* getFilename(char* pathname) {
    for(int i = strlen(pathname) - 1; i >= 0; i--){
        if(pathname[i] == '/'){
            return pathname + i + 1;
        }
    }
    return pathname;
}

struct dir_entry createEntry(short inum, char *filename) {
    struct dir_entry entry;
    entry.inum = inum;
    memset(&entry.name, '\0', DIRNAMELEN);
    int filename_len = strlen(filename);
    if(filename_len > DIRNAMELEN) { filename_len = DIRNAMELEN; } 
    memcpy(&entry.name, filename, filename_len);
    
    return entry;
}

int addEntry(short inum, struct dir_entry new_entry) {
    // get the prent inode
    INODE_INFO *inode_info = get_use_inode(inum);
    if (inode_info == NULL) {
        TracePrintf(1, "addEntry() cannot get inode!\n");
        return ERROR;
    }
    if (inode_info->val->type != INODE_DIRECTORY) {
        TracePrintf(1, "addEntry() the given inode is not a directory!\n");
        return ERROR;
    }

    // get the entry inode
    INODE_INFO *entry_inode_info = get_use_inode(new_entry.inum);
    if (inode_info == NULL) {
        TracePrintf(1, "addEntry() cannot get inode of new entry!\n");
        return ERROR;
    }

    int size = inode_info->val->size;
    int cur_pos = 0;
    struct dir_entry cur_entry;

    // while loop
    while (cur_pos < size) {
        int res = ReadHandler(inum, cur_pos, &cur_entry, sizeof(struct dir_entry));
        if (res == -1) {
            TracePrintf(1, "addEntry() ReadHandler error!\n");
            return ERROR;
        }
        if (cur_entry.inum == 0) { // TODO sync with rmdir
            break;
        }
        cur_pos += sizeof(struct dir_entry);
    }

    int res = WriteHandler(inum, cur_pos, &new_entry, sizeof(struct dir_entry));
    if (res == -1) {
        TracePrintf(1, "addEntry() WriteHandler error\n");
        return ERROR;
    }

    inode_info->isDirty = 1;
    entry_inode_info->val->nlink++;
    entry_inode_info->isDirty = 1;

    return res;
}

short createFile(char *filename, short parent_inum, int file_type) {
    short exist_inum = findInumInDir(filename, parent_inum);
    if (exist_inum == -1) {
        TracePrintf(1, "createFile() find filename error!\n");
        return ERROR;
    }
    else if (exist_inum > 0) {
        TracePrintf(1, "createFile() filename already exist\n");
        return ERROR;
    }

    INODE_INFO *new_inode_info = get_new_inode();
    if (new_inode_info == NULL) {
        TracePrintf(1, "createFile() cannot create new inode!\n");
        return ERROR;
    }

    struct dir_entry new_entry = createEntry(new_inode_info->inodeNum, filename);

    // TODO add entry
    if (addEntry(parent_inum, new_entry) == -1) {
        set_bitmap_free(inode_bitmap, new_inode_info->inodeNum,  NUM_INODES);
        return ERROR;
    }
    new_inode_info->val->type = file_type;
    new_inode_info->val->nlink = 0;
    new_inode_info->val->size = 0;
    new_inode_info->val->reuse++;
    new_inode_info->isDirty = 1;
    BLOCK_INFO* binfo = get_block(INODE_TO_BLOCK(new_inode_info->inodeNum));
    binfo->isDirty = 1;

    return new_inode_info->inodeNum; // inum
}

int MessageHandler(char *msg, int pid)
{
    int code = (unsigned char)msg[0];
    int res = 0;
    switch (code)
    {
    case CALL_OPEN:
        /* code */
        break;
    case CALL_CREATE:
        /* code */
        break;
    case CALL_READ:
        short   inum = *(short*)(msg+1);
        int     pos = *(int*)(msg+3);
        void    *buf = *(void**)(msg + 7);
        int     read_len = *(int*)(msg+15);

        void* tmp_buf = malloc(read_len);
        res = ReadHandler(inum, pos, tmp_buf, read_len);
        if (res == -1) {
            TracePrintf(1, "ReadHandler() error!\n");
        }
        else {
            CopyTo(pid, buf, tmp_buf, res);
        }
        free(tmp_buf);
        break;
    case CALL_WRITE:
        short   inum = *(short*)(msg+1);
        int     pos = *(int*)(msg+3);
        void    *buf = *(void**)(msg + 7);
        int     write_len = *(int*)(msg+15);

        void* tmp_buf = malloc(write_len);
        CopyFrom(pid, tmp_buf, buf, write_len);
        res = WriteHandler(inum, pos, tmp_buf, read_len);
        if (res == -1) {
            TracePrintf(1, "ReadHandler() error!\n");
        }
        free(tmp_buf);
        break;
    case CALL_SEEK:
        /* code */
        short   inum = *(short*)(msg+1);
        res = SeekHandler(inum);
        if (res == -1) {
            TracePrintf(1, "SeekHandler() error!\n");
        }
        break;
    case CALL_LINK:
        /* code */
        break;
    case CALL_UNLINK:
        /* code */
        break;
    case CALL_SYMLINK:
        /* code */
        break;
    case CALL_READLINK:
        /* code */
        break;
    case CALL_MKDIR:
        /* code */
        break;
    case CALL_RMDIR:
        /* code */
        break;
    case CALL_CHDIR:
        /* code */
        break;
    case CALL_STAT:
        /* code */
        break;
    case CALL_SYNC:
        /* code */
        res = SyncHandler();
        if (res == -1) {
            TracePrintf(1, "SyncHandler() error!\n");
        }
        break;
    case CALL_SHUTDOWN:
        /* code */
        break;
    default:
        TracePrintf(1, "MessageHandler() Unrecognized code!\n");
        break;
    }
    return res;
}

int OpenHandler(char *pathname, short cur_dir_idx)
{
    return resolvePath(pathname,cur_dir_idx);
}

int CreateHandler(char *pathname, short cur_dir_idx)
{
    short parent_inum = getParentInum(pathname, cur_dir_idx);
    if (parent_inum == -1) {
        TracePrintf(1,"CreateHandler() cannot find parent inode\n");
        return ERROR;
    }
    char* filename = getFilename(pathname);

    return 0;
}

int ReadHandler(short inum, int position, void *buf, int size)
{
    INODE_INFO *cur_inode_info = get_use_inode(inum);
    if (cur_inode_info == NULL) {
        TracePrintf(1,"ReadHandler() cannot find inode\n");
        return ERROR;
    }

    int total_read_count = 0;
    int req_left = 0, file_left = 0, block_left = 0, read_count = 0;
    int block_num = 0, block_offset = 0;
    struct inode* cur_inode = cur_inode_info->val;

    while (
        total_read_count < size 
        && position + total_read_count < cur_inode->size
    ) {
        block_num = (position + total_read_count) / BLOCKSIZE;
        block_offset = (position + total_read_count) % BLOCKSIZE;
        char* block_content = NULL;
        if (block_num < NUM_DIRECT) {
            // direct block
            // get block node using inode->direct[blocknum]
            BLOCK_INFO *binfo = get_block(cur_inode->direct[block_num]);
            block_content = &binfo->data[block_offset];
        }
        else {
            // indirect
            if (block_num >= NUM_DIRECT + BLOCKSIZE/sizeof(int)) { return ERROR;} 
            // get block node using inode->indirect
            BLOCK_INFO *indirectbinfo = get_block(cur_inode->indirect);
            // TODO: handle error

            int *indirect_blocks = (int *)indirectbinfo->data;
            int bnum = indirect_blocks[block_num - NUM_DIRECT];

            BLOCK_INFO *binfo = get_block(bnum);
            // TODO: handle error
            
            block_content = &binfo->data[block_offset];
        }
        // get block_contnet of block's content using block_offset

        req_left = size - total_read_count;
        file_left = cur_inode->size - (position + total_read_count);
        block_left = BLOCKSIZE - block_offset; 
            
        read_count = req_left;
        if (read_count > file_left) read_count = file_left;
        if (read_count > block_left) read_count = block_left;

        memcpy((char*)buf + total_read_count, block_content, read_count);
        total_read_count += read_count;
    }
    return total_read_count;
}

int WriteHandler(short inum, int position, void *buf, int size)
{
    INODE_INFO *cur_inode_info = get_use_inode(inum);
    if (cur_inode_info == NULL) {
        TracePrintf(1,"WriteHandler() cannot find inode\n");
        return ERROR;
    }

    cur_inode_info->isDirty = 1;

    struct inode* cur_inode = cur_inode_info->val;
    if (position + size > BLOCKSIZE * (cur_inode->size/ BLOCKSIZE + 1)) {
        // need to assign new block 
        extend(cur_inode_info, position + size);
    }

    int total_write_count = 0;
    int req_left = 0, block_left = 0, write_count = 0;
    int block_num = 0, block_offset = 0;

    while (
        total_write_count < size 
        && position + total_write_count < cur_inode->size
    ) {
        block_num = (position + total_write_count) / BLOCKSIZE;
        block_offset = (position + total_write_count) % BLOCKSIZE;
        char* block_content = NULL;
        if (block_num < NUM_DIRECT) {
            // direct block
            // get block node using inode->direct[blocknum]
            BLOCK_INFO *binfo = get_block(cur_inode->direct[block_num]);
            // TODO: handle error

            binfo->isDirty = 1;
            block_content = &binfo->data[block_offset];
        }
        else {
            // indirect
            if (block_num >= NUM_DIRECT + BLOCKSIZE/sizeof(int)) { return ERROR;} 
            // get block node using inode->indirect
            BLOCK_INFO *indirectbinfo = get_block(cur_inode->indirect);
            // TODO: handle error
            
            int *indirect_blocks = (int *)indirectbinfo->data;
            int bnum = indirect_blocks[block_num - NUM_DIRECT];

            BLOCK_INFO *binfo = get_block(bnum);
            // TODO: handle error

            binfo->isDirty = 1;
            block_content = &binfo->data[block_offset];
        }

        req_left = size - total_write_count;
        block_left = BLOCKSIZE - block_offset; 
            
        write_count = req_left;
        if (write_count > block_left) write_count = block_left;

        memcpy(block_content, (char*)buf + total_write_count, write_count);
        total_write_count += write_count;
    }
    return total_write_count;
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
    return sync();
}

int ShutdownHandler()
{
    // SyncHandler()
    // This request performs an orderly shutdown of the file server process. 
    // All dirty cached inodes and disk blocks should be written back to the disk 
    // (as in a Sync request), and the file server should then call Exit to complete its shutdown. 
    // As part of a Shutdown request, the server should print an informative message indicating that it is shutting down. 
    // This request always returns the value 0.

    return 0;
}

int SeekHandler(short inodeNum){
    INODE_INFO* info = get_inode(inodeNum);
    if(info->inodeNum == -1){
	    TracePrintf(0, "in SeekHandler...inode number is 0\n");
	    return ERROR;
    }
    return info->val->size;
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




int sync() {
    // This request writes all dirty cached inodes back to their corresponding disk blocks (in the cache) 
    // and then writes all dirty cached disk blocks to the disk. 
    // The request does not complete until all dirty inodes and disk blocks have been written to the disk; 
    // this request always then returns the value 0.
    INODE_INFO* cur_inode = inode_head;
    BLOCK_INFO* cur_block = block_head;
    while(cur_inode != inode_tail) {
        int inodeNum = cur_inode->inodeNum;
        if(cur_inode->isDirty == 1) {
            int blockNum = INODE_TO_BLOCK(inodeNum);
            int offset = INODE_IN_BLOCK_ADDR(inodeNum);
            BLOCK_INFO* tmp_block  = get_block(blockNum);
            /*
            if(tmp_block == NULL) {
                char data[BLOCKSIZE];
                ReadSector(blockNum, (void*)(data));
                memcpy(data + offset, cur_inode->val, sizeof(struct inode));
                int sig = WriteSector(blockNum, (void*)(data));
                if(sig == 0) {
         	        printf("in sync()...WriteSector Error\n");
                }
            }else{
                memcpy((void*)(tmp_block->data + offset), cur_inode->val, sizeof(struct inode));
                tmp_block->isDirty = 1;
            }
            */
            memcpy(tmp_block->data + offset, cur_inode->val, sizeof(struct inode));
            tmp_block->isDirty = 1;
            cur_inode->isDirty = 0;
        }
        cur_inode = cur_inode->next;
    }
  
    while(cur_block != block_tail) {
        if(cur_block->isDirty == 1) {
            int sig = WriteSector(cur_block->blockNum, (void*)(cur_block->data));
            if(sig == 0) {
                cur_block->isDirty = 0;
            }else{
                printf("in sync()...WriteSector Error\n");
                return -1;
            } 
            cur_block->isDirty = 0;
        }
        cur_block = cur_block->next;
    }
    return 0;
  }