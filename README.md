# Custom Block-Based Filesystem 
This project is a small, educational filesystem implemented from scratch in C.
It simulates a block device, manages storage using inodes and a free-space bitmap, and exposes a simple interactive shell for creating, deleting, reading, and writing files inside a virtual disk image.

The goal of the project was to understand how real filesystems track metadata, allocate blocks, and maintain on-disk structures without relying on OS-level libraries or existing FS logic.
