// libDisk.c
#include "libDisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BLOCKSIZE 256

static int disk_fd = -1;

// Function to get the size of the disk
int getDiskSize(int fd) {
    off_t currentPos = lseek(fd, 0, SEEK_CUR);
    if (currentPos == (off_t)-1) {
        perror("Error getting current file position");
        return -1;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == (off_t)-1) {
        perror("Error getting file size");
        return -1;
    }

    if (lseek(fd, currentPos, SEEK_SET) == (off_t)-1) {
        perror("Error restoring file position");
        return -1;
    }

    return size;
}

// Function to open the disk
int openDisk(char *filename, int nBytes) {
    if (filename == NULL || nBytes < 0) {
        return -1;
    }

    /* reading from an existing disk */
    if (nBytes == 0) {
        disk_fd = open(filename, O_RDWR);
        if (disk_fd == -1) {
            perror("Error opening file");
            return -1;
        }
    } else {
        if (nBytes < BLOCKSIZE) {
            perror("nBytes less than BLOCKSIZE");
            return -1;
        }

        disk_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (disk_fd == -1) {
            perror("Error creating file");
            return -1;
        }

        nBytes = (nBytes / BLOCKSIZE) * BLOCKSIZE;

        char emptyBlock[BLOCKSIZE] = {0};
        for (int i = 0; i < nBytes / BLOCKSIZE; i++) {
            if (write(disk_fd, emptyBlock, BLOCKSIZE) != BLOCKSIZE) {
                perror("Error initializing disk");
                close(disk_fd);
                return -1;
            }
        }
    }

    return disk_fd;
}

// Function to close the disk
int closeDisk(int disk) {
    if (disk != disk_fd) {
        printf("Error: Invalid disk descriptor\n");
        return -1;
    }
    if (close(disk) < 0) {
        perror("Error closing disk");
        return -1;
    }
    disk_fd = -1;
    return 0;
}

// Function to read a block from the disk
int readBlock(int disk, int bNum, void *block) {
    if (disk != disk_fd || bNum < 0 || block == NULL) {
        printf("Error: Invalid parameters\n");
        return -1;
    }

    int diskSize = getDiskSize(disk);
    if (diskSize < 0) {
        printf("Error: Unable to get disk size\n");
        return -1;
    }

    if (bNum * BLOCKSIZE >= diskSize) {
        printf("Error: Block number out of range\n");
        return -1;
    }

    if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0) {
        perror("Failed to seek to the block position");
        return -1;
    }

    ssize_t bytesRead = read(disk, block, BLOCKSIZE);
    if (bytesRead < 0) {
        perror("Failed to read the block");
        return -1;
    }
    if (bytesRead != BLOCKSIZE) {
        memset((char *)block + bytesRead, 0, BLOCKSIZE - bytesRead);
    }
    return 0;
}

// Function to write a block to the disk
int writeBlock(int disk, int bNum, void *block) {
    if (disk != disk_fd || bNum < 0 || block == NULL) {
        printf("Error: Invalid parameters\n");
        return -1;
    }

    int diskSize = getDiskSize(disk);
    if (diskSize < 0) {
        printf("Error: Unable to get disk size\n");
        return -1;
    }

    if (bNum * BLOCKSIZE >= diskSize) {
        printf("Error: Block number out of range\n");
        return -1;
    }

    if (lseek(disk, bNum * BLOCKSIZE, SEEK_SET) < 0) {
        perror("Failed to seek to the block position");
        return -1;
    }

    ssize_t bytesWritten = write(disk, block, BLOCKSIZE);
    if (bytesWritten < 0) {
        perror("Failed to write the block");
        return -1;
    }
    if (bytesWritten != BLOCKSIZE) {
        printf("Error: Wrote %zd bytes instead of %d bytes\n", bytesWritten, BLOCKSIZE);
        return -1;
    }
    return 0;
}

// Function to check if the disk exists
int diskExists(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return 0; 
    }
    close(fd);
    return 1; 
}
