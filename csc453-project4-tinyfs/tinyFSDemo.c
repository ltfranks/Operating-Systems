// tinyFSDemo.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"
#include "tinyFS_errno.h"

// this is a random test that i am making
// not sure if it tests properly

// ANSI escape codes for colors
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define NUM_TEST_DISKS 4 /* number of disks to test with */
#define BLOCKSIZE 256
#define NUM_BLOCKS 50 /* total number of blocks on each disk */
#define NUM_TEST_BLOCKS 10
#define TEST_BLOCKS {25,39,8,9,15,21,25,33,35,42}


int main() {
    printf(YELLOW "Creating TinyFS file system...\n" RESET);
    if (tfs_mkfs(DEFAULT_DISK_NAME, DEFAULT_DISK_SIZE) != TFS_SUCCESS) {
        printf(RED "Failed to create file system\n" RESET);
        return 1;
    }
    printf(GREEN "File system created successfully\n\n" RESET);

    printf(YELLOW "Mounting TinyFS file system...\n" RESET);
    if (tfs_mount(DEFAULT_DISK_NAME) != TFS_SUCCESS) {
        printf(RED "Failed to mount file system\n" RESET);
        return 1;
    }
    printf(GREEN "File system mounted successfully\n\n" RESET);

    printf(YELLOW "Displaying file system fragments before any file written...\n" RESET);
    if (tfs_displayFragments() != TFS_SUCCESS) {
        printf(RED "Failed to display fragments\n" RESET);
        return 1;
    }
    printf("\n");

    printf(YELLOW "Creating a new file 'file1'...\n" RESET);
    fileDescriptor fd = tfs_openFile("file1");
    if (fd < 0) {
        printf(RED "Failed to open file\n" RESET);
        return 1;
    }
    printf(GREEN "File 'file1' created successfully\n\n" RESET);

    printf(YELLOW "Writing content to 'file1'...\n" RESET);
    char data[BLOCKSIZE] = "Hello, TinyFS!";
    if (tfs_writeFile(fd, data, sizeof(data)) != TFS_SUCCESS) {
        printf(RED "Failed to write file\n" RESET);
        return 1;
    }
    printf(GREEN "Content written to 'file1' successfully\n\n" RESET);

// could work on reading content. Right now it just prints out whats
// in the buffer from tfs_readByte.  
    printf(YELLOW "Reading content from 'file1'...\n" RESET);
    char buffer[BLOCKSIZE];
    if (tfs_readByte(fd, buffer) != TFS_SUCCESS) {
        printf(RED "Failed to read file\n" RESET);
        return 1;
    }
    printf(CYAN "File content: %s\n\n" RESET, buffer);

    printf(YELLOW "Closing 'file1'...\n" RESET);
    if (tfs_closeFile(fd) != TFS_SUCCESS) {
        printf(RED "Failed to close file\n" RESET);
        return 1;
    }
    printf(GREEN "File 'file1' closed successfully\n\n" RESET);

    printf(YELLOW "Renaming 'file1' to 'newfile1'...\n" RESET);
    if (tfs_rename(fd, "newfile1") != TFS_SUCCESS) {
        printf(RED "Failed to rename file\n" RESET);
        return 1;
    }
    printf(GREEN "File renamed successfully\n\n" RESET);

    printf(YELLOW "Listing directory contents...\n" RESET);
    if (tfs_readdir() != TFS_SUCCESS) {
        printf(RED "Failed to read directory\n" RESET);
        return 1;
    }
    printf("\n");

    // Displaying the fragments right after the file is made doesnt work
    // only the first two blocks are USED
    printf(YELLOW "Displaying file system fragments...\n" RESET);
    if (tfs_displayFragments() != TFS_SUCCESS) {
        printf(RED "Failed to display fragments\n" RESET);
        return 1;
    }
    printf("\n");

    // After defragmenting, the used block for FILE1 shows but,
    // it should be stored starting at BLOCK 4. Not block 3
    printf(YELLOW "Defragmenting the file system...\n" RESET);
    if (tfs_defrag() != TFS_SUCCESS) {
        printf(RED "Failed to defragment\n" RESET);
        return 1;
    }
    printf(GREEN "Defragmentation completed successfully\n\n" RESET);

    printf(YELLOW "Displaying file system fragments after defragmentation...\n" RESET);
    if (tfs_displayFragments() != TFS_SUCCESS) {
        printf(RED "Failed to display fragments\n" RESET);
        return 1;
    }
    printf("\n");

    printf(YELLOW "Making 'newfile1' read-only...\n" RESET);
    if (tfs_makeRO("newfile1") != TFS_SUCCESS) {
        printf(RED "Failed to make file read-only\n" RESET);
        return 1;
    }
    printf(GREEN "File 'newfile1' is now read-only\n\n" RESET);

    printf(YELLOW "Attempting to write to read-only file...\n" RESET);
    if (tfs_writeByte(fd, 0, 'X') == TFS_SUCCESS) {
        printf(RED "Successfully wrote to read-only file (should have failed)\n" RESET);
    } else {
        printf(GREEN "Failed to write to read-only file (expected)\n" RESET);
    }
    printf("\n");

    printf(YELLOW "Making 'newfile1' read-write...\n" RESET);
    if (tfs_makeRW("newfile1") != TFS_SUCCESS) {
        printf(RED "Failed to make file read-write\n" RESET);
        return 1;
    }
    printf(GREEN "File 'newfile1' is now read-write\n\n" RESET);

    printf(YELLOW "Writing a byte to 'newfile1'...\n" RESET);
    if (tfs_writeByte(fd, 0, 'X') != TFS_SUCCESS) {
        printf(RED "Failed to write byte to file\n" RESET);
        return 1;
    }
    printf(GREEN "Byte written successfully\n\n" RESET);

    // read content of file after byte was written to file
    // ...

    printf(YELLOW "Reading file info for 'newfile1'...\n" RESET);
    if (tfs_readFileInfo(fd) != TFS_SUCCESS) {
        printf(RED "Failed to read file info\n" RESET);
        return 1;
    }
    printf("\n");

    printf(YELLOW "Unmounting TinyFS file system...\n" RESET);
    if (tfs_unmount() != TFS_SUCCESS) {
        printf(RED "Failed to unmount file system\n" RESET);
        return 1;
    }
    printf(GREEN "File system unmounted successfully\n\n" RESET);

    printf("\nend of demo\n\n");
    return 0;
}