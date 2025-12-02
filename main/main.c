#include "fs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_help() {
    printf("Commands:\n");
    printf("  format <sizeMB>\n");
    printf("  mount <disk.img>\n");
    printf("  create <name> <size>\n");
    printf("  delete <name>\n");
    printf("  write <name> <text...>\n");
    printf("  read <name>\n");
    printf("  list\n");
    printf("  exit\n");
}


int main() {
    char cmd[256];

    printf("MyFS Interactive Shell\n");
    printf("Type 'help' for commands.\n\n");

    while (1) {
        printf("myfs> ");
        if (!fgets(cmd, sizeof(cmd), stdin))
            break;

        // remove trailing newline
        cmd[strcspn(cmd, "\n")] = 0;

        if (strncmp(cmd, "help", 4) == 0) {
            print_help();
        }

        else if (strncmp(cmd, "format", 6) == 0) {
            int size;
            if (sscanf(cmd, "format %d", &size) == 1) {
                fs_format("disk.img", size * 1024 * 1024);
            } else {
                printf("Usage: format <sizeMB>\n");
            }
        }

        else if (strncmp(cmd, "mount", 5) == 0) {
            char name[128];
            if (sscanf(cmd, "mount %s", name) == 1) {
                fs_mount(name);
            } else {
                printf("Usage: mount <disk.img>\n");
            }
        }

        else if (strncmp(cmd, "create", 6) == 0) {
            char name[64];
            int size;
            if (sscanf(cmd, "create %s %d", name, &size) == 2) {
                fs_create(name, size);
            } else {
                printf("Usage: create <filename> <size>\n");
            }
        }

        else if (strncmp(cmd, "list", 4) == 0) {
            extern inode_t *inode_table;
            printf("Files:\n");
            for (int i = 0; i < 128; i++) {
                if (inode_table[i].used) {
                    printf("  %d: %s (%u bytes)\n", i, inode_table[i].name, inode_table[i].size);
                }
            }
        }
        else if (strncmp(cmd, "delete", 6) == 0) {
            char name[64];
            if (sscanf(cmd, "delete %63s", name) == 1) {
                fs_delete(name);
            } else {
                printf("Usage: delete <filename>\n");
            }
        }

        else if (strncmp(cmd, "write", 5) == 0) {
            char name[64];
            char text[512];

            // parse: write name some text here...
            if (sscanf(cmd, "write %63s %511[^\n]", name, text) == 2) {
                uint32_t len = (uint32_t)strlen(text);
                fs_write(name, (const uint8_t *)text, len);
            } else {
                printf("Usage: write <filename> <text>\n");
            }
        }

        else if (strncmp(cmd, "read", 4) == 0) {
            char name[64];
            if (sscanf(cmd, "read %63s", name) == 1) {
                uint8_t buf[1024];
                int n = fs_read(name, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';   // VERY IMPORTANT
                    printf("Read %d bytes from '%s'\n", n, name);
                    printf("Contents of '%s':\n%s\n", name, (char *)buf);
                }
            } else {
                printf("Usage: read <filename>\n");
            }
        }


        else if (strncmp(cmd, "diskmap", 7) == 0) {
            extern uint8_t *free_bitmap;
            extern superblock_t sb;

            printf("DISK MAP:\n");
            for (uint32_t i = 0; i < sb.total_blocks; i++) {
                if (i % 64 == 0)
                    printf("\n%04u: ", i);
                printf(free_bitmap[i] ? "#" : ".");
            }
            printf("\n");
        }

        else if (strncmp(cmd, "exit", 4) == 0) {
            break;
        }

        else if (strlen(cmd) > 0) {
            printf("Unknown command. Type 'help'.\n");
        }
    }

    return 0;
}
