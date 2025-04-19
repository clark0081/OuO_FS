// #include <comp421/yalnix.h>
// #include <comp421/hardware.h>
// #include <comp421/filesystem.h>
// #include <comp421/iolib.h>

#include "yalnix.h"
#include "hardware.h"
#include "filesystem.h"
#include "iolib.h"
#include <string.h>

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

