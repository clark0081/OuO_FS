// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>

#include "yalnix.h"
#include "hardware.h"
#include "filesystem.h"
#include "iolib.h"
#include <string.h>

// for message passing data structure
#define YFS_PID             1
#define CALL_OPEN           1
#define CALL_CLOSE          2
#define CALL_CREATE         3
#define CALL_READ           4
#define CALL_WRITE          5
#define CALL_SEEK           6
#define CALL_LINK           7
#define CALL_UNLINK         8
#define CALL_SYMLINK        9
#define CALL_READLINK       10
#define CALL_MKDIR          11
#define CALL_RMDIR          12
#define CALL_CHDIR          13
#define CALL_STAT           14
#define CALL_SYNC           15
#define CALL_SHUTDOWN       16


int MessageHandler(char *msg, int pid);

int ReadHandler(short inum, int position, void *buf, int size);
int WriteHandler(short inum, int position, void *buf, int size);

int SymLinkHandler(char *oldname, char *newname, short cur_dir_idx);
int ReadLinkHandler(char *pathname, char *buf, int len, short cur_dir_idx);
int MkDirHandler(char *pathname, short cur_dir_idx);
int RmDirHandler(char *pathname, short cur_dir_idx);
int ChDirHandler(char *pathname, short cur_dir_idx);
int StatHandler(char *pathname, struct Stat *statbuf, short cur_dir_idx);
int SyncHandler(void);
int ShutdownHandler(void);




