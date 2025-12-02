#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE *disk_file = NULL;
superblock_t sb;
uint8_t *free_bitmap = NULL;
inode_t *inode_table = NULL;


static int fs_find_inode(const char *name) {
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        if (inode_table[i].used &&
            strncmp(inode_table[i].name, name, 64) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int fs_format(const char *disk_path, uint32_t size_bytes) {

    FILE *f = fopen(disk_path, "wb+");
    if (!f) {
        printf("Error: could not create disk image.\n");
        return -1;
    }

    fseek(f, size_bytes - 1, SEEK_SET);
    fwrite("\0", 1, 1, f);

    sb.magic            = 0xDEADBEEF;
    sb.block_size       = 4096;
    sb.total_blocks     = size_bytes / sb.block_size;
    sb.inode_table_start = 2;
    sb.inode_count      = 128;
    sb.data_block_start = 5;

    fseek(f, 0, SEEK_SET);
    fwrite(&sb, sizeof(sb), 1, f);


    uint8_t bitmap[4096];
    memset(bitmap, 0, sizeof(bitmap));

    bitmap[0] = 1; 
    bitmap[1] = 1; 
    bitmap[2] = 1; 
    bitmap[3] = 1; 
    bitmap[4] = 1; 

    fseek(f, 4096 * 1, SEEK_SET);
    fwrite(bitmap, sizeof(bitmap), 1, f);

    inode_t empty;
    memset(&empty, 0, sizeof(empty));

    fseek(f, 4096 * 2, SEEK_SET);
    for (int i = 0; i < 128; i++) {
        fwrite(&empty, sizeof(empty), 1, f);
    }

    fclose(f);
    printf("Disk formatted: %s (%u bytes)\n", disk_path, size_bytes);
    return 0;
}
int fs_mount(const char *disk_path) {

    disk_file = fopen(disk_path, "rb+");
    if (!disk_file) {
        printf("Error: could not open disk image.\n");
        return -1;
    }

    fseek(disk_file, 0, SEEK_SET);
    if (fread(&sb, sizeof(sb), 1, disk_file) != 1) {
        printf("Error: could not read superblock.\n");
        return -1;
    }

    if (sb.magic != 0xDEADBEEF) {
        printf("Error: invalid filesystem magic number.\n");
        return -1;
    }

    free_bitmap = malloc(sb.block_size);
    if (!free_bitmap) {
        printf("Memory error.\n");
        return -1;
    }

    fseek(disk_file, sb.block_size * 1, SEEK_SET);
    fread(free_bitmap, sb.block_size, 1, disk_file);

    inode_table = malloc(sb.inode_count * sizeof(inode_t));
    if (!inode_table) {
        printf("Memory error.\n");
        return -1;
    }

    fseek(disk_file, sb.block_size * sb.inode_table_start, SEEK_SET);
    fread(inode_table, sizeof(inode_t), sb.inode_count, disk_file);

    printf("Filesystem mounted successfully.\n");
    printf("Total blocks: %u\n", sb.total_blocks);

    return 0;
}
int alloc_block(void) {
    for (uint32_t i = sb.data_block_start; i < sb.total_blocks; i++) {
        if (free_bitmap[i] == 0) {     // block is free
            free_bitmap[i] = 1;        // mark it used
            return i;                  // return block index
        }
    }
    return -1; // no free blocks available
}

void free_block(int block_index) {
    uint32_t b = (uint32_t) block_index;

    if (b >= sb.data_block_start && b < sb.total_blocks) {
        free_bitmap[b] = 0;
    }
}

int fs_create(const char *name, uint32_t size) {

    // 1. Find a free inode
    int idx = -1;
    for (uint32_t i = 0; i < sb.inode_count; i++) {
        if (inode_table[i].used == 0) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        printf("Error: no free inodes.\n");
        return -1;
    }

    inode_t *node = &inode_table[idx];

    // 2. Initialize inode
    node->used = 1;
    strncpy(node->name, name, 63);
    node->name[63] = '\0';
    node->size = size;
    memset(node->blocks, 0, sizeof(node->blocks));

    // 3. Determine how many blocks the file needs
    uint32_t block_size = sb.block_size;
    uint32_t blocks_needed = (size + block_size - 1) / block_size;

    if (blocks_needed > 10) {
        printf("Error: file too large (max 10 blocks).\n");
        node->used = 0;
        return -1;
    }

    // 4. Allocate blocks
    for (uint32_t i = 0; i < blocks_needed; i++) {
        int b = alloc_block();
        if (b < 0) {
            printf("Error: out of disk space.\n");
            node->used = 0;
            return -1;
        }
        node->blocks[i] = b;
    }

    // 5. Write inode table back to disk
    fseek(disk_file, sb.block_size * sb.inode_table_start, SEEK_SET);
    fwrite(inode_table, sizeof(inode_t), sb.inode_count, disk_file);

    // Also write updated free block bitmap
    fseek(disk_file, sb.block_size * 1, SEEK_SET);
    fwrite(free_bitmap, sb.block_size, 1, disk_file);

    printf("Created file '%s' (%u bytes) using inode %d\n", name, size, idx);
    return 0;
}
int fs_delete(const char *name) {
    int idx = fs_find_inode(name);
    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return -1;
    }

    inode_t *node = &inode_table[idx];

    // free all blocks
    for (int i = 0; i < 10; i++) {
        if (node->blocks[i] != 0) {
            free_block((int)node->blocks[i]);
            node->blocks[i] = 0;
        }
    }

    node->used = 0;
    node->name[0] = '\0';
    node->size = 0;

    // write back inode table
    fseek(disk_file, sb.block_size * sb.inode_table_start, SEEK_SET);
    fwrite(inode_table, sizeof(inode_t), sb.inode_count, disk_file);

    // write back updated bitmap
    fseek(disk_file, sb.block_size * 1, SEEK_SET);
    fwrite(free_bitmap, sb.block_size, 1, disk_file);

    printf("Deleted file '%s' (inode %d)\n", name, idx);
    return 0;
}
int fs_write(const char *name, const uint8_t *data, uint32_t size) {

printf("[DEBUG] fs_write: disk_file=%p\n", disk_file);
printf("[DEBUG] fs_write: sb.block_size=%u sb.data_block_start=%u\n",
        sb.block_size, sb.data_block_start);

    int idx = fs_find_inode(name);
    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return -1;
    }

    inode_t *node = &inode_table[idx];
    uint32_t block_size = sb.block_size;

    // limit write to what we can store in allocated blocks
    uint32_t max_bytes = 10 * block_size;
    if (size > max_bytes) {
        printf("Error: write too large (max %u bytes for this file).\n", max_bytes);
        size = max_bytes;
    }

    node->size = size;

    uint32_t remaining = size;
    const uint8_t *p = data;

    for (int i = 0; i < 10 && remaining > 0; i++) {
        uint32_t blk = node->blocks[i];
        if (blk == 0) break; // no more blocks allocated

        uint32_t to_write = remaining > block_size ? block_size : remaining;
        long offset = (long)blk * (long)block_size;  // blk is a physical block index

        fseek(disk_file, offset, SEEK_SET);
        fwrite(p, 1, to_write, disk_file);

        // zero the rest of the block so old data doesn't linger
        if (to_write < block_size) {
            static uint8_t zeros[4096];
            memset(zeros, 0, block_size - to_write);
            fwrite(zeros, 1, block_size - to_write, disk_file);
        }

        p += to_write;
        remaining -= to_write;
    }

    // write updated inode table back
    fseek(disk_file, sb.block_size * sb.inode_table_start, SEEK_SET);
    fwrite(inode_table, sizeof(inode_t), sb.inode_count, disk_file);

    printf("Wrote %u bytes to '%s'\n", size, name);
    return 0;
}
int fs_read(const char *name, uint8_t *buffer, uint32_t size) {
    int idx = fs_find_inode(name);
    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return -1;
    }

    inode_t *node = &inode_table[idx];
    uint32_t block_size = sb.block_size;

    if (size > node->size) {
        size = node->size;  // don't read past EOF
    }

    uint32_t remaining = size;
    uint8_t *p = buffer;

    for (int i = 0; i < 10 && remaining > 0; i++) {
        uint32_t blk = node->blocks[i];
        if (blk == 0) break;

        uint32_t to_read = remaining > block_size ? block_size : remaining;
        long offset = (long)blk * (long)block_size;

        fseek(disk_file, offset, SEEK_SET);
        fread(p, 1, to_read, disk_file);

        p += to_read;
        remaining -= to_read;
    }

    return (int)size;
}