#include "fs.h"
#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FILES 128

// simple struct to map list rows -> inode indices
typedef struct {
    int inode_index;
} FileEntry;

static FileEntry file_entries[MAX_FILES];
static int file_count = 0;
static int selected_file = 0;

static void build_file_list(void) {
    file_count = 0;
    for (uint32_t i = 0; i < sb.inode_count && file_count < MAX_FILES; i++) {
        if (inode_table[i].used) {
            file_entries[file_count].inode_index = (int)i;
            file_count++;
        }
    }
    if (selected_file >= file_count) selected_file = file_count - 1;
    if (selected_file < 0) selected_file = 0;
}

static void draw_ui(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // clear screen
    erase();

    // top title
    mvprintw(0, 0, "MyFS Viewer  |  disk blocks: %u  block size: %u  (q = quit, arrows = move)",
             sb.total_blocks, sb.block_size);

    // left side: file list
    int list_width = cols / 3;
    mvprintw(2, 0, "Files:");
    for (int i = 0; i < file_count && i < rows - 4; i++) {
        int inode_idx = file_entries[i].inode_index;
        inode_t *node = &inode_table[inode_idx];

        if (i == selected_file) {
            attron(A_REVERSE);
        }
        mvprintw(3 + i, 0, "%3d: %-20s (%u bytes)",
                 inode_idx,
                 node->name[0] ? node->name : "<noname>",
                 node->size);
        if (i == selected_file) {
            attroff(A_REVERSE);
        }
    }

    // right side: block map
    int map_x = list_width + 2;
    int map_y = 2;
    mvprintw(map_y, map_x, "Block map (S=super, B=bitmap, I=inodes, #=used, .=free, *=selected-file)");

    // compute selected file blocks (if any)
    int selected_inode = -1;
    if (file_count > 0 && selected_file >= 0 && selected_file < file_count) {
        selected_inode = file_entries[selected_file].inode_index;
    }

    int selected_blocks[10];
    int sel_count = 0;
    if (selected_inode >= 0) {
        inode_t *node = &inode_table[selected_inode];
        for (int i = 0; i < 10; i++) {
            if (node->blocks[i] != 0) {
                selected_blocks[sel_count++] = (int)node->blocks[i];
            }
        }
    }

    // simple grid: 64 blocks per row
    int blocks_per_row = 64;
    int start_row = map_y + 2;
    for (uint32_t b = 0; b < sb.total_blocks; b++) {
        int r = (int)(b / blocks_per_row);
        int c = (int)(b % blocks_per_row);

        int y = start_row + r;
        int x = map_x + c;

        char ch = '.';

        // classify metadata
        if (b == 0) ch = 'S';            // superblock
        else if (b == 1) ch = 'B';       // bitmap
        else if (b >= sb.inode_table_start && b < sb.data_block_start) ch = 'I';
        else {
            int is_selected = 0;
            for (int i = 0; i < sel_count; i++) {
                if (selected_blocks[i] == (int)b) {
                    is_selected = 1;
                    break;
                }
            }
            if (is_selected) ch = '*';
            else if (free_bitmap[b]) ch = '#'; // used
            else ch = '.';
        }

        mvaddch(y, x, ch);
    }

    // info about selected file
    int info_y = rows - 4;
    if (selected_inode >= 0) {
        inode_t *node = &inode_table[selected_inode];
        mvprintw(info_y, 0, "Selected inode %d  name='%s'  size=%u",
                 selected_inode,
                 node->name[0] ? node->name : "<noname>",
                 node->size);
        mvprintw(info_y + 1, 0, "Blocks: ");
        int col = 9;
        for (int i = 0; i < 10; i++) {
            if (node->blocks[i] != 0) {
                mvprintw(info_y + 1, col, "%u ", node->blocks[i]);
                col += 5;
            }
        }
    } else {
        mvprintw(info_y, 0, "No files.");
    }

    refresh();
}

int main(int argc, char *argv[]) {
    const char *disk = "disk.img";
    if (argc >= 2) {
        disk = argv[1];
    }

    if (fs_mount(disk) != 0) {
        fprintf(stderr, "Failed to mount %s\n", disk);
        return 1;
    }

    build_file_list();

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    draw_ui();
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case KEY_UP:
                if (selected_file > 0) selected_file--;
                break;
            case KEY_DOWN:
                if (selected_file < file_count - 1) selected_file++;
                break;
            default:
                break;
        }
        draw_ui();
    }

    endwin();
    return 0;
}
