// libTinyFS.c
#include "libTinyFS.h"
#include "tinyFS_errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC_NUMBER 0x44

typedef struct {
    char filename[9]; // 8 characters + NULL terminator
    int file_size;
    int first_block;
    int read_only;
    time_t creation_time;
    time_t modification_time;
    time_t access_time;
} inode;

typedef struct {
    int magic_number;
    int root_inode;
    int free_block;
} superblock;

static superblock sb;
static inode inodes[BLOCKSIZE / sizeof(inode)];
static int disk_fd = -1;
static int mounted = 0;

int tfs_mkfs(char *filename, int nBytes) {
    if (nBytes < BLOCKSIZE) {
        return TFS_ERROR;
    }

    int fd = openDisk(filename, nBytes);
    if (fd < 0) {
        return TFS_ERROR;
    }

    memset(inodes, 0, sizeof(inodes));
    sb.magic_number = MAGIC_NUMBER;
    sb.root_inode = 0;
    sb.free_block = 1; 

    if (writeBlock(fd, 0, &sb) < 0) {
        closeDisk(fd);
        return TFS_ERROR;
    }

    if (writeBlock(fd, 1, inodes) < 0) {
        closeDisk(fd);
        return TFS_ERROR;
    }

    closeDisk(fd);
    return TFS_SUCCESS;
}

int tfs_mount(char *diskname) {
    if (mounted) {
        return TFS_ERROR;
    }

    disk_fd = openDisk(diskname, 0);
    if (disk_fd < 0) {
        printf("Error: Failed to open disk\n");
        return TFS_ERROR;
    }

    if (readBlock(disk_fd, 0, &sb) < 0) {
        printf("Error: Failed to read superblock\n");
        closeDisk(disk_fd);
        disk_fd = -1;
        return TFS_ERROR;
    }

    if (sb.magic_number != MAGIC_NUMBER) {
        printf("Error: Magic number mismatch. Expected: %x, Found: %x\n", MAGIC_NUMBER, sb.magic_number);
        closeDisk(disk_fd);
        disk_fd = -1;
        return TFS_ERROR;
    }

    if (readBlock(disk_fd, 1, inodes) < 0) {
        printf("Error: Failed to read inodes\n");
        closeDisk(disk_fd);
        disk_fd = -1;
        return TFS_ERROR;
    }

    mounted = 1;
    return TFS_SUCCESS;
}

int tfs_unmount(void) {
    if (!mounted) {
        return TFS_ERROR;
    }

    if (writeBlock(disk_fd, 0, &sb) < 0) {
        return TFS_ERROR;
    }

    if (writeBlock(disk_fd, 1, inodes) < 0) {
        return TFS_ERROR;
    }

    if (closeDisk(disk_fd) < 0) {
        return TFS_ERROR;
    }

    disk_fd = -1;
    mounted = 0;
    return TFS_SUCCESS;
}

fileDescriptor tfs_openFile(char *name) {
    if (!mounted) {
        return TFS_ERROR;
    }

    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (strcmp(inodes[i].filename, name) == 0) {
            return i;
        }
    }

    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (inodes[i].filename[0] == 0) {
            strncpy(inodes[i].filename, name, 8);
            inodes[i].filename[8] = '\0';
            inodes[i].file_size = 0;
            inodes[i].first_block = -1;
            inodes[i].read_only = 0;
            inodes[i].creation_time = time(NULL);
            inodes[i].modification_time = time(NULL);
            inodes[i].access_time = time(NULL);
            return i;
        }
    }

    return TFS_ERROR;
}

int tfs_closeFile(fileDescriptor FD) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode)) {
        return TFS_ERROR;
    }

    return TFS_SUCCESS;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode) || inodes[FD].read_only) {
        return TFS_ERROR;
    }

    int blocks_needed = (size + BLOCKSIZE - 1) / BLOCKSIZE;
    int current_block = inodes[FD].first_block;

    for (int i = 0; i < blocks_needed; i++) {
        if (current_block == -1) {
            current_block = sb.free_block;
            sb.free_block++;
            if (i == 0) {
                inodes[FD].first_block = current_block;
            }
        }

        if (writeBlock(disk_fd, current_block, buffer + i * BLOCKSIZE) < 0) {
            return TFS_ERROR;
        }

        current_block++;
    }

    inodes[FD].file_size = size;
    inodes[FD].modification_time = time(NULL);
    inodes[FD].access_time = time(NULL);

    return TFS_SUCCESS;
}

int tfs_deleteFile(fileDescriptor FD) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode) || inodes[FD].read_only) {
        return TFS_ERROR;
    }

    memset(&inodes[FD], 0, sizeof(inode));
    return TFS_SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode)) {
        return TFS_ERROR;
    }

    if (inodes[FD].file_size == 0) {
        return TFS_ERROR;
    }

    int block_num = inodes[FD].first_block;
    if (readBlock(disk_fd, block_num, buffer) < 0) {
        return TFS_ERROR;
    }

    inodes[FD].access_time = time(NULL);

    return TFS_SUCCESS;
}

int tfs_seek(fileDescriptor FD, int offset) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode)) {
        return TFS_ERROR;
    }

    if (offset < 0 || offset > inodes[FD].file_size) {
        return TFS_ERROR;
    }

    inodes[FD].first_block = offset / BLOCKSIZE;
    return TFS_SUCCESS;
}


int tfs_displayFragments(void) {
    if (!mounted) {
        return TFS_ERROR;
    }

    int num_blocks = DEFAULT_DISK_SIZE / BLOCKSIZE;
    int block_status[num_blocks];
    memset(block_status, 0, sizeof(block_status));

    // mark superblock and inode table as used
    block_status[0] = 1;
    block_status[1] = 1;

    // mark file blocks as used
    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (inodes[i].filename[0] != 0) {
            int current_block = inodes[i].first_block;
            int file_blocks = (inodes[i].file_size + BLOCKSIZE - 1) / BLOCKSIZE;
            for (int j = 0; j < file_blocks; j++) {
                if (current_block != -1) {
                    block_status[current_block] = 1;
                    current_block++;
                }
            }
        }
    }

    // display block status
    for (int i = 0; i < num_blocks; i++) {
        printf("Block %d: %s\n", i, block_status[i] ? "Used" : "Free");
    }

    return TFS_SUCCESS;
}


int tfs_defrag(void) {
    if (!mounted) {
        return TFS_ERROR;
    }

    int num_blocks = DEFAULT_DISK_SIZE / BLOCKSIZE;
    int block_status[num_blocks];
    memset(block_status, 0, sizeof(block_status));

    // mark superblock and inode table as used
    block_status[0] = 1;
    block_status[1] = 1;

    // move file blocks to contiguous locations
    int next_free_block = 2;
    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (inodes[i].filename[0] != 0) {
            int current_block = inodes[i].first_block;
            int file_blocks = (inodes[i].file_size + BLOCKSIZE - 1) / BLOCKSIZE;
            int new_first_block = next_free_block;

            for (int j = 0; j < file_blocks; j++) {
                if (current_block != -1) {
                    char buffer[BLOCKSIZE];
                    if (readBlock(disk_fd, current_block, buffer) < 0) {
                        return TFS_ERROR;
                    }
                    if (writeBlock(disk_fd, next_free_block, buffer) < 0) {
                        return TFS_ERROR;
                    }
                    block_status[next_free_block] = 1;
                    current_block++;
                    next_free_block++;
                }
            }

            // update the first_block of the inode
            inodes[i].first_block = new_first_block;
        }
    }

    // update free block pointer in superblock
    sb.free_block = next_free_block;

    return TFS_SUCCESS;
}

int tfs_rename(fileDescriptor FD, char* newName) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode) || inodes[FD].filename[0] == 0) {
        return TFS_ERROR;
    }

    strncpy(inodes[FD].filename, newName, 8);
    inodes[FD].filename[8] = '\0';
    return TFS_SUCCESS;
}

int tfs_readdir(void) {
    if (!mounted) {
        return TFS_ERROR;
    }

    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (inodes[i].filename[0] != 0) {
            printf("File: %s, Size: %d bytes\n", inodes[i].filename, inodes[i].file_size);
        }
    }

    return TFS_SUCCESS;
}

int tfs_makeRO(char *name) {
    if (!mounted) {
        return TFS_ERROR;
    }

    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (strcmp(inodes[i].filename, name) == 0) {
            inodes[i].read_only = 1;
            return TFS_SUCCESS;
        }
    }

    return TFS_ERROR;
}

int tfs_makeRW(char *name) {
    if (!mounted) {
        return TFS_ERROR;
    }

    for (int i = 0; i < sizeof(inodes) / sizeof(inode); i++) {
        if (strcmp(inodes[i].filename, name) == 0) {
            inodes[i].read_only = 0;
            return TFS_SUCCESS;
        }
    }

    return TFS_ERROR;
}

int tfs_writeByte(fileDescriptor FD, int offset, unsigned int data) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode) || inodes[FD].read_only) {
        return TFS_ERROR;
    }

    if (offset < 0 || offset >= inodes[FD].file_size) {
        return TFS_ERROR;
    }

    int block_num = inodes[FD].first_block + offset / BLOCKSIZE;
    int byte_offset = offset % BLOCKSIZE;

    char block[BLOCKSIZE];
    if (readBlock(disk_fd, block_num, block) < 0) {
        return TFS_ERROR;
    }

    block[byte_offset] = (char)data;

    if (writeBlock(disk_fd, block_num, block) < 0) {
        return TFS_ERROR;
    }

    inodes[FD].modification_time = time(NULL);
    inodes[FD].access_time = time(NULL);

    return TFS_SUCCESS;
}

int tfs_readFileInfo(fileDescriptor FD) {
    if (!mounted || FD < 0 || FD >= sizeof(inodes) / sizeof(inode)) {
        return TFS_ERROR;
    }

    printf("File: %s\n", inodes[FD].filename);
    printf("Size: %d bytes\n", inodes[FD].file_size);
    printf("Creation Time: %s", ctime(&inodes[FD].creation_time));
    printf("Modification Time: %s", ctime(&inodes[FD].modification_time));
    printf("Access Time: %s", ctime(&inodes[FD].access_time));

    return TFS_SUCCESS;
}
