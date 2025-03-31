#ifndef LIBTINYFS_H
#define LIBTINYFS_H

#include "libDisk.h"
#include "tinyFS_errno.h"
#include <time.h>

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 2560
#define DEFAULT_DISK_NAME "tinyFSDisk"

typedef int fileDescriptor;

int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *diskname);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);

int tfs_displayFragments(void);
int tfs_defrag(void);
int tfs_rename(fileDescriptor FD, char* newName);
int tfs_readdir(void);
int tfs_makeRO(char *name);
int tfs_makeRW(char *name);
int tfs_writeByte(fileDescriptor FD, int offset, unsigned int data);
int tfs_readFileInfo(fileDescriptor FD);

#endif // LIBTINYFS_H
