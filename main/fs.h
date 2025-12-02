#ifndef FS_H
#define FS_H

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t inode_table_start;
    uint32_t inode_count;
    uint32_t data_block_start;
} superblock_t;

typedef struct {
    uint8_t  used;
    char     name[64];
    uint32_t size;
    uint32_t blocks[10];
} inode_t;

// globals defined in fs.c (make sure they are NOT static there)
extern superblock_t sb;
extern uint8_t *free_bitmap;
extern inode_t *inode_table;

// fs operations
int fs_format(const char *disk_path, uint32_t size_bytes);
int fs_mount(const char *disk_path);
int alloc_block(void);
void free_block(int block_index);
int fs_create(const char *name, uint32_t size);
int fs_delete(const char *name);
int fs_write(const char *name, const uint8_t *data, uint32_t size);
int fs_read(const char *name, uint8_t *buffer, uint32_t size);

#endif
