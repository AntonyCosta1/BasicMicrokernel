#ifndef FS_H
#define FS_H

#include <stdint.h>

#define TREEFS_MAGIC 0x54524653  // "TRFS" in ASCII

#define MAX_BLOCKS 1024
#define MAX_INODES 128
#define BLOCK_SIZE 512

#define FS_FILE 1
#define FS_DIRECTORY 2

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t block_size;
} superblock_t;

typedef struct {
    uint32_t size;
    uint32_t type;
    uint32_t links;
    uint32_t blocks[8];
} inode_t;

typedef struct {
    uint32_t inode;
    char name[32];
} dir_entry_t;

int fs_init(void);

inode_t *path_lookup(const char *path);

int mkdir(const char *path);
int create(const char *path);
int unlink(const char *path);
int ls(const char *path);
int read(int fd, void *buf, uint32_t size);
int write(int fd, const void *buf, uint32_t size);

#endif // FS_H


