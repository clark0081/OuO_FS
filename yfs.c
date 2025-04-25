#include "yfs.h"
#include "block.h"
#include "lrucache.h"
#include <string.h>


//TODO
// 1. fix reuse
// 2. MAXPATHNAMELEN


short findInumInDir(char * filename, short dir) {
    struct inode_info* dir_inode = get_use_inode(dir);
    if (dir_inode == NULL) {
        TracePrintf(1, "findInumInDir() cannot get inode for %s\n", filename);
        return ERROR;
    }

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
        TracePrintf(1,"cur %s\n",pathname);
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
        if (inum == -1 || inum == 0) {
            TracePrintf(1, "resolvePath() cannot find files %s in directory %d for %s\n", node_name, cur_dir, org_pathname);
            return ERROR;
        }
        INODE_INFO* next_inode_info = get_use_inode(inum);

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
        TracePrintf(1, "addEntry() error in getting info!\n");
        return ERROR;
    }
    if (inode_info->val->type != INODE_DIRECTORY) {
        TracePrintf(1, "addEntry() the given inode is not a directory!\n");
        return ERROR;
    }

    // get the entry inode
    INODE_INFO *entry_inode_info = get_use_inode(new_entry.inum);
    if (entry_inode_info == NULL) {
        TracePrintf(1, "addEntry() cannot get inode of new entry!\n");
        return ERROR;
    }

    int size = inode_info->val->size;
    int cur_pos = 0;
    struct dir_entry entry;

    // while loop
    while (cur_pos < (int)(size - sizeof(struct dir_entry))) {
        if (ReadHandler(inum, cur_pos, &entry, sizeof(struct dir_entry)) == -1) {
            TracePrintf(1, "addEntry() ReadHandler error!\n");
            return ERROR;
        }
        if (entry.inum == 0) {
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
        // return ERROR;
        INODE_INFO *exist_inode_info = get_use_inode(exist_inum);
        exist_inode_info->val->size = 0;
        return exist_inum;
    }

    INODE_INFO *new_inode_info = get_new_inode();
    if (new_inode_info == NULL) {
        TracePrintf(1, "createFile() cannot create new inode!\n");
        return ERROR;
    }

    struct dir_entry new_entry = createEntry(new_inode_info->inodeNum, filename);

    new_inode_info->val->type = file_type;
    new_inode_info->val->nlink = 0;
    new_inode_info->val->size = 0;
    new_inode_info->val->reuse++;
    if (addEntry(parent_inum, new_entry) == -1) {
        new_inode_info->val->reuse--;
        free_inode(new_inode_info->inodeNum);
        return ERROR;
    }
    TracePrintf(1, "createFile() create new inode! %d\n", new_inode_info->inodeNum);
    return new_inode_info->inodeNum; // inum
}

int MessageHandler(char *msg, int pid)
{
    int code = (unsigned char)msg[0];
    int res = 0;
    TracePrintf(1, " code %d\n", code);
    switch (code)
    {
    case CALL_OPEN: {
        /* code */
        void    *pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);
        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';
        res = OpenHandler(pathname,cur_dir);
        if (res == -1) {
            TracePrintf(1, "OpenHandler() error!\n");
        }
        free(pathname);
        break;
    }
    case CALL_CREATE: {
        /* code */
        void    *pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);
        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';
        res = CreateHandler(pathname, cur_dir);
        if (res == -1) {
            TracePrintf(1, "CreateHandler() error!\n");
        }
        free(pathname);
        break;
    }
    case CALL_READ: {
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
    }
    case CALL_WRITE: {
        short   inum = *(short*)(msg+1);
        int     pos = *(int*)(msg+3);
        void    *buf = *(void**)(msg + 7);
        int     write_len = *(int*)(msg+15);

        void* tmp_buf = malloc(write_len);
        CopyFrom(pid, tmp_buf, buf, write_len);
        res = WriteHandler(inum, pos, tmp_buf, write_len);
        if (res == -1) {
            TracePrintf(1, "WriteHandler() error!\n");
        }
        free(tmp_buf);
        break;
    }
    case CALL_SEEK: {
        /* code */
        short   inum = *(short*)(msg+1);
        res = SeekHandler(inum);
        if (res == -1) {
            TracePrintf(1, "SeekHandler() error!\n");
        }
        break;
    }
    case CALL_LINK: {
        /* code */
        void* oldname_addr = *(void**)(msg + 1);
        int     oldname_len = *(int*)(msg+9);
        void* newname_addr = *(void**)(msg + 13);
        int     newname_len = *(int*)(msg+21);
        short   cur_dir = *(short*)(msg+25);

        char* oldname = (char*)malloc(oldname_len + 1);
        CopyFrom(pid, (void*)oldname, oldname_addr, oldname_len);
        oldname[oldname_len] = '\0';

        char* newname = (char*)malloc(newname_len + 1);
        CopyFrom(pid, (void*)newname, newname_addr, newname_len);
        newname[newname_len] = '\0';

        res = LinkHandler(oldname, newname, cur_dir);
        free(oldname);
        free(newname);
        break;
    }
    case CALL_UNLINK: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';

        res = UnlinkHandler(pathname, cur_dir);
        free(pathname);
        break;
    }
    case CALL_SYMLINK: {
        void* oldname_addr = *(void**)(msg + 1);
        int     oldname_len = *(int*)(msg+9);
        void* newname_addr = *(void**)(msg + 13);
        int     newname_len = *(int*)(msg+21);
        short   cur_dir = *(short*)(msg+25);
    
        char* oldname = (char*)malloc(oldname_len + 1);
        CopyFrom(pid, (void*)oldname, oldname_addr, oldname_len);
        oldname[oldname_len] = '\0';

        char* newname = (char*)malloc(newname_len + 1);
        CopyFrom(pid, (void*)newname, newname_addr, newname_len);
        newname[newname_len] = '\0';

        res = SymLinkHandler(oldname, newname, cur_dir);
        free(oldname);
        free(newname);
        break;
    }
    case CALL_READLINK: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        void* buf_addr = *(void**)(msg + 13);
        int     buf_len = *(int*)(msg+21);
        short   cur_dir = *(short*)(msg+25);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';

        void* tmp_buf = malloc(buf_len);
        res = ReadLinkHandler(pathname,tmp_buf, buf_len, cur_dir);
        if (res == -1) {
            TracePrintf(1, "ReadLinkHandler() error!\n");
        }
        else {
            CopyTo(pid, buf_addr, tmp_buf, res);
        }
        free(tmp_buf);
        free(pathname);
        break;
    }
    case CALL_MKDIR: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';

        res = MkDirHandler(pathname, cur_dir);
        free(pathname);
        break;
    }
    case CALL_RMDIR: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';

        res = RmDirHandler(pathname, cur_dir);
        free(pathname);
        break;
    }
    case CALL_CHDIR: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        short   cur_dir = *(short*)(msg+13);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';

        res = ChDirHandler(pathname, cur_dir);
        free(pathname);
        break;
    }
    case CALL_STAT: {
        void* pathname_addr = *(void**)(msg + 1);
        int     pathname_len = *(int*)(msg+9);
        void* stat_buf = *(void**)(msg + 13);
        short   cur_dir = *(short*)(msg+21);

        char* pathname = (char*)malloc(pathname_len + 1);
        CopyFrom(pid, (void*)pathname, pathname_addr, pathname_len);
        pathname[pathname_len] = '\0';
        void* tmp_buf = malloc(sizeof(struct Stat));
        res = StatHandler(pathname, tmp_buf, cur_dir);
        if (res == -1) {
            TracePrintf(1, "StatHandler() error!\n");
        }
        else {
            CopyTo(pid, stat_buf, tmp_buf, sizeof(struct Stat));
        }
        free(tmp_buf);
        free(pathname);
        break;
    }
    case CALL_SYNC: {
        /* code */
        res = SyncHandler();
        if (res == -1) {
            TracePrintf(1, "SyncHandler() error!\n");
        }
        break;
    }
    case CALL_SHUTDOWN: {
        /* code */
        res = ShutdownHandler();
        if (res == -1) {
            TracePrintf(1, "ShutdownHandler() error!\n");
        }
	    if(res == 0){
		
		    int i;
		    for(i = 0; i < MESSAGE_SIZE; i++){
		        msg[i] = '\0';
		    }
		    memcpy(msg, &res, sizeof(res));
		    Reply(msg, pid);      
            printf("------------------------------------------------------------------\n");
            printf("Shutdown request received. Terminating Yalnix File System....\n");
            printf("GoodBye ~~(｡•́︿•̀｡)   \n");
		    printf("------------------------------------------------------------------\n");
		    Exit(0);
	    }
        break;
    }
    default: {
        TracePrintf(1, "MessageHandler() Unrecognized code!\n");
        break;
    }
    }
    return res;
}

int OpenHandler(char *pathname, short cur_dir_idx)
{
    int res = resolvePath(pathname, cur_dir_idx);
    TracePrintf(1, "OpenHandler() res %d!\n", res);
    return res;
    
}

int CreateHandler(char *pathname, short cur_dir_idx)
{
    if (pathname[strlen(pathname) - 1] == '/') {
        TracePrintf(1,"CreateHandler() path name should not end with \'/\' \n");
        return ERROR;
    }

    short parent_inum = getParentInum(pathname, cur_dir_idx);
    if (parent_inum == -1) {
        TracePrintf(1,"CreateHandler() cannot find parent inode\n");
        return ERROR;
    }
    char* filename = getFilename(pathname);

    return createFile(filename, parent_inum, INODE_REGULAR);
}

int ReadHandler(short inum, int position, void *buf, int size)
{
    // This request reads data from an open file, beginning at the current position in the file as represented by the given file descriptor fd. 
    // The argument fd specifies the file descriptor number of the file to be read, buf specifies the address of the buffer in the requesting process 
    // into which to perform the read, and size is the number of bytes to be read from the file. 
    // This request returns the number of bytes read, which will be 0 if reading at the end-of-file; 
    // the number of bytes read will be the minimum of the number of bytes requested and the number of bytes remaining in the file before the end-of-file. 
    // Upon successful completion, the current position in the file as represented by the given file descriptor fd should be advanced by the number of bytes read, 
    // and only this number of bytes within the caller�s buf should be modified (bytes beyond this count must remain unchanged). 
    // On any error, this call should return ERROR. It is not an error to attempt to Read() from a file descriptor that is open on a directory. 
    // Unless this Read operation returns ERROR, the current position for subsequent Read or Write operations on this file descriptor 
    // advances by the number of bytes read (the value returned by the Read request).
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
            if (block_num >= (int)(NUM_DIRECT + BLOCKSIZE/sizeof(int))) { return ERROR;} 
            // get block node using inode->indirect
            BLOCK_INFO *indirectbinfo = get_block(cur_inode->indirect);

            int *indirect_blocks = (int *)indirectbinfo->data;
            int bnum = indirect_blocks[block_num - NUM_DIRECT];

            BLOCK_INFO *binfo = get_block(bnum);
            
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

            binfo->isDirty = 1;
            block_content = &binfo->data[block_offset];
        }
        else {
            // indirect
            if (block_num >= (int)(NUM_DIRECT + BLOCKSIZE/sizeof(int))) { return ERROR;} 
            // get block node using inode->indirect
            BLOCK_INFO *indirectbinfo = get_block(cur_inode->indirect);

            int *indirect_blocks = (int *)indirectbinfo->data;
            int bnum = indirect_blocks[block_num - NUM_DIRECT];

            BLOCK_INFO *binfo = get_block(bnum);

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

int LinkHandler(char *oldname, char *newname, short cur_dir_idx)
{
    // The file oldname must not be a directory. 
    short old_inum = resolvePath(oldname, cur_dir_idx);
    if (old_inum == -1) {
        TracePrintf(1,"LinkHandler() error in resolve oldname path\n");
        return ERROR;
    }
    else if (old_inum == 0) {
        TracePrintf(1,"LinkHandler() oldname does not exist\n");
        return ERROR;
    }
    INODE_INFO *old_info = get_use_inode(old_inum);
    if (old_info == NULL) {
        TracePrintf(1,"LinkHandler() error in getting info\n");
        return ERROR;
    }
    if (old_info->val->type == INODE_DIRECTORY) {
        TracePrintf(1,"LinkHandler() oldname is a directory\n");
        return ERROR;
    }
    // It is an error if the file newname already exists. 
    short new_inum = resolvePath(newname, cur_dir_idx);
    if (new_inum == -1) {
        TracePrintf(1,"LinkHandler() error in resolve newname path\n");
        return ERROR;
    }
    else if (new_inum > 0) {
        TracePrintf(1,"LinkHandler() newname already exists\n");
        return ERROR;
    }

    // On success, this request returns 0; on
    // any error, the value ERRORis returned.
    short parent_inum = getParentInum(newname, cur_dir_idx);
    char* filename = getFilename(newname);
    addEntry(parent_inum, createEntry(old_inum, filename));
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

    
    short parent_inum = getParentInum(newname, cur_dir_idx);
    char* filename = getFilename(newname);
    short inum = createFile(filename, parent_inum, INODE_SYMLINK);
    if(inum == ERROR) {
        TracePrintf(1,"SymLinkHandler()  fail to create symlink\n");
        return ERROR; 
    }

    if (WriteHandler(inum, 0, oldname, strlen(oldname)) == -1) {
        TracePrintf(1,"SymLinkHandler()  write oldname error\n");
        return ERROR; 
    }

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
    // This request reads the name of the file that the symbolic link pathname is linked to; 
    // the named pathname must be that of a symbolic link. 
    // On success, this request returns the length (number of characters) of the name that the symbolic link pathname points to 
    // (or the value len, whichever is smaller), and places in the buffer beginning with address buf the name that the symbolic link points to, 
    // up to a maximum number of characters of len characters; 
    // if the name that the symbolic link points to is longer than len bytes, 
    // the name is truncated as returned in the buffer buf (this is not an error). 
    // The characters placed into buf are not terminated by a �\0� character. 
    // On any error, the value ERROR is returned.

    short parent_inum = getParentInum(pathname, cur_dir_idx);
    char* filename = getFilename(pathname);
    short inum = findInumInDir(filename,parent_inum);
    return ReadHandler(inum, 0, buf, len);
    
}

int MkDirHandler(char *pathname, short cur_dir_idx)
{
    /*
        - create a new file under parent_dir
        - add dir_entries of ".", ".." to the new index node
    */
    short parent_inum = getParentInum(pathname, cur_dir_idx);
    if (parent_inum == -1) {
        TracePrintf(1,"CreateHandler() cannot find parent inode\n");
        return ERROR;
    }
    char* filename = getFilename(pathname);
    short inum = createFile(filename, parent_inum, INODE_DIRECTORY);
    if(inum == ERROR) { 
        TracePrintf(1,"MkDirHandler()  fail to create directory\n");
        return ERROR; 
    }

    addEntry(inum, createEntry(inum, "."));
    addEntry(inum, createEntry(parent_inum, ".."));
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
    // This request deletes the existing directory named pathname. 
    // The directory must contain no directory entries other than the ��.�� and ��..�� 
    // entries and possibly some free entries. 
    // On success, this request returns 0; 
    // on any error, the value ERROR is returned. 
    // Note that it is an error to attempt to remove the root directory; 
    // it is also an error to attempt to remove individually the ��.�� or ��..�� entry from a directory.
    if(strcmp(pathname, "/") || strcmp(pathname, ".") || strcmp(pathname, "..") == 0){
        TracePrintf(0, "in RmDirHandler()...attempting to remove root or . or ..\n");
        return ERROR;
    }
    size_t len = strlen(pathname);
    int need_dot = (len > 0 && pathname[len - 1] == '/');

    size_t new_len = len + (need_dot ? 1 : 0) + 1;
    char* new_path = (char*)malloc(new_len);

    if (!new_path) {
        TracePrintf(0, "in RmDirHandler()... malloc failed\n");
        perror("malloc failed");
        return ERROR;
    }
    strcpy(new_path, pathname);  
    if (need_dot) {
        new_path[len] = '.';     
        new_path[len + 1] = '\0';
    }
    short inodeNum = resolvePath(new_path, cur_dir_idx);
    if(inodeNum == -1){
        TracePrintf(0, "in RmDirHandler()...resolvePath failed\n");
        free(new_path);
        return ERROR;
    }
    INODE_INFO* info = get_use_inode(inodeNum);
    if(info->val->type != INODE_DIRECTORY){
        TracePrintf(0, "in RmDirHandler()...not a directory\n");
        free(new_path);
        return ERROR;
    }

    int i;
    for(i = 2; i < (int)(info->val->size / sizeof(struct dir_entry)); i++){
	    struct dir_entry entry;
        ReadHandler(inodeNum, i * sizeof(entry), (void*)&entry, sizeof(entry));
	    if(entry.inum != 0){
            TracePrintf(0, "in RmDirHandler()...directory is not empty\n");
            free(new_path);
	        return ERROR;
	    }
    }
    short parent_inodeNum = getParentInum(new_path, cur_dir_idx);
    if(parent_inodeNum == -1){
        TracePrintf(0, "in RmDirHandler()...getParentInum fail\n");
        free(new_path);
        return ERROR;
    }
    ///////////////////////////////
    INODE_INFO* parent_info = get_use_inode(parent_inodeNum);
    if(parent_info->inodeNum == -1){
        TracePrintf(0, "in RmDirHandler()...cannot find parent inode\n");
        free(new_path);
        return ERROR;
    }
    struct dir_entry entry;
    for(i = 2; i < (int)(parent_info->val->size / sizeof(struct dir_entry)); i++){
	    if(ReadHandler(parent_inodeNum, i * sizeof(entry), (void*)&entry, sizeof(entry)) == -1){
            TracePrintf(0, "in RmDirHandler()...cannot find entry parent inode\n");
            free(new_path);
	        return ERROR;
	    }
	    if(inodeNum == entry.inum){
	        struct dir_entry remove = createEntry(0, "\0");
            WriteHandler(inodeNum, i * sizeof(struct dir_entry), &remove, sizeof(struct dir_entry));
            info->val->type = INODE_FREE;    
            set_bitmap_free(inode_bitmap, inodeNum, NUM_INODES);
            info->isDirty = 1;
	        break;
	    }
    }
    free(new_path);
    return 0;
}

int UnlinkHandler(char* pathname, short cur_dir_idx) {

    short parent_inum = getParentInum(pathname, cur_dir_idx);
    if (parent_inum == -1) {
        TracePrintf(1,"UnlinkHandler() cannot find parent inode\n");
        return ERROR;
    }
    char* filename = getFilename(pathname);
    // loop through parent's data to find file name
    INODE_INFO* parent_info = get_use_inode(parent_inum);
    int inum = -1;
    INODE_INFO * node_info = NULL;
    struct dir_entry entry;
    for (int pos = 0; pos < (int)(parent_info->val->size - sizeof(struct dir_entry)); pos += sizeof(struct dir_entry)) {
        if (ReadHandler(parent_inum, pos, &entry, sizeof(struct dir_entry)) == -1) {
            TracePrintf(1, "UnlinkHandler() ReadHandler error!\n");
            return ERROR;
        }

        if (strncmp(filename, entry.name, DIRNAMELEN) == 0) {
            inum = entry.inum;
            node_info = get_use_inode(inum);
            if (node_info->val->type == INODE_DIRECTORY) {
                TracePrintf(1, "UnlinkHandler() pathname is a directory!\n");
                return ERROR;
            }
            // null dir_entry
            struct dir_entry null_entry = createEntry(0, "\0");
            if (WriteHandler(parent_inum, pos, &null_entry, sizeof(struct dir_entry)) == -1) {
                TracePrintf(1, "UnlinkHandler() fail to clear dir_entry!\n");
                return ERROR;
            }
            break;  
        }
    }
    // check inode nlink
    node_info->val->nlink--;
    if (node_info->val->nlink <= 0) {
        // delete inode and block
        free_inode(inum);
    }

    return 0;
}

int ChDirHandler(char *pathname, short cur_dir_idx)
{
    /*
        - get the index node
            handle error
        - return node's index
    */
    // This request changes the current directory within the requesting process to be the directory indicated by pathname . 
    // The current directory of a process should be remembered by the inode number of that directory, within the file system library in that process. 
    // That current directory inode number should then be passed to the file server on each request that takes any file name arguments. 
    // The file pathname on this request must be a directory. On success, this request returns 0; on any error, the value ERROR is returned.
    size_t len = strlen(pathname);
    int need_dot = (len > 0 && pathname[len - 1] == '/');

    size_t new_len = len + (need_dot ? 1 : 0) + 1;
    char* new_path = (char*)malloc(new_len);

    if (!new_path) {
        TracePrintf(0, "in ChDirHandler()... malloc failed\n");
        perror("malloc failed");
        return ERROR;
    }
    strcpy(new_path, pathname);  
    if (need_dot) {
        new_path[len] = '.';     
        new_path[len + 1] = '\0';
    }
    short inodeNum = resolvePath(new_path, cur_dir_idx);
    if(inodeNum == -1){
        TracePrintf(0, "in ChDirHandler()...resolvePath failed\n");
        free(new_path);
        return ERROR;
    }
    INODE_INFO* info = get_use_inode(inodeNum);
    if (info == NULL) {
        TracePrintf(0, "in ChDirHandler() get node error\n");
        free(new_path);
        return ERROR;
    }   
    if(info->val->type != INODE_DIRECTORY){
        TracePrintf(0, "in ChDirHandler()...not a directory\n");
        free(new_path);
        return ERROR;
    }
    free(new_path);
    return info->inodeNum;
}

int StatHandler(char *pathname, struct Stat *statbuf, short cur_dir_idx)
{
    /*
        - get the index node
            handle error
        - fill index, type, size, nlink to statbuf
    */
    short inum = resolvePath(pathname, cur_dir_idx);
    if (inum <= 0) {
        TracePrintf(1,"StatHandler()  error in resolve path or file not exist\n");
        return ERROR; 
    }

    INODE_INFO *cur_node_info = get_use_inode(inum);
    if (cur_node_info == NULL) {
        TracePrintf(1,"StatHandler()  error in getting info\n");
        return ERROR; 
    }
    statbuf->inum = cur_node_info->inodeNum;
    statbuf->nlink = cur_node_info->val->nlink;
    statbuf->size = cur_node_info->val->size;
    statbuf->type = cur_node_info->val->type;

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
    sync();
    
    // free hash table
    // free inode list
    // free block list
    // free bitmap
    free(block_bitmap);
    free(inode_bitmap);
    block_bitmap = NULL;
    inode_bitmap = NULL;
    clear_inode_block_hashTable();
    clear_inode_cache();
    clear_block_cache();
    return 0;
}

int SeekHandler(short inodeNum){
    INODE_INFO* info = get_use_inode(inodeNum);
    if(info->inodeNum == -1){
	    TracePrintf(0, "in SeekHandler...inode number is 0\n");
	    return ERROR;
    }
    return info->val->size;
}

int main(int argc, char** argv) {
    (void)argc;
    init_lru();
    init_inode_block();
    Register(FILE_SERVER);
    
    print_bitmap(inode_bitmap,NUM_INODES);
    int pid = Fork();
    if (pid == 0) {
        // child process
        TracePrintf(0, "child start\n");
        Exec(argv[1], argv + 1);
        Halt();
    }
    while(1) {
        char msg[MESSAGE_SIZE];
        int pid = Receive(msg);
        if (pid == 0) {
            // deadlock
            TracePrintf(0, "Receive() return 0 to avoid deadlock\n");
            Exit(0);
        }
        else if (pid == -1) {
            // error
            TracePrintf(0, "Receive() return -1 due to error\n");
            Exit(0);
        }
        int res = MessageHandler(msg, pid); // pid for using CopyFrom()/ CopyTo()

        for (int i = 0; i < MESSAGE_SIZE; i++) { msg[i] = '\0'; }  // clean the msg
        memcpy(msg, &res, sizeof(int));
        Reply(msg, pid);
    }
    return 0;
}
