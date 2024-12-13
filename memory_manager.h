#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#define BlockAllocated 1
#define BlockFree 0
#define lastBlockForFile -1


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
typedef struct memory_block
{
    size_t size;
    int free; // 1 indicates the block is free, 0 used
    void *offset; // Pointer to the block's start in the memory pool
    struct memory_block *next;
    pthread_mutex_t lock; // lock for each block
} memory_block;




void mem_init(size_t size);

void* mem_alloc(size_t size);

void mem_free(void *ptr);

void* mem_resize(void *block, size_t size);

void mem_deinit();

void display_memory();

#endif