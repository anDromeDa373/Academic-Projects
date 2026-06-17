#include "db_engine.h"
#include <string.h>
#include <stdlib.h> 

#define MAX_NAME_LEN 31

void pool_init(MemoryPool *pool, size_t num_blocks, size_t block_size) {
    size_t i;
    // Handling invalid arguments
    if (!pool || num_blocks == 0 || block_size == 0) {
        // If pool is not NULL, reset all members to NULL or 0 to mark it as unusable
        if (pool) {
            pool->memory = NULL;
            pool->blocks = NULL;
            pool->free_stack = NULL;
            pool->block_size = 0;
            pool->num_blocks = 0;
        }
        return;
    }
    // Dynamically allocate memory for the entire pool
    pool->memory = malloc(num_blocks * block_size);
    if (!pool->memory) {
        pool->blocks = NULL;
        pool->free_stack = NULL;
        pool->block_size = 0;
        pool->num_blocks = 0;
    }
    // Allocate memory to store block metadata
    pool->blocks = malloc(num_blocks * sizeof(Block));
    if (!pool->blocks) {
        // The previously allocated memory must be freed to avoid a memory leak
        free(pool->memory); 
        pool->memory = NULL;
        pool->free_stack = NULL;
        pool->block_size = 0;
        pool->num_blocks = 0;
        return;
    }
    
    for (i = 0; i < num_blocks; i++) {
        // Assign the starting address of the i-th block's usable memory area
        // Cast to (char*) to ensure byte-wise pointer arithmetic
        pool->blocks[i].data = (char*)pool->memory + (i * block_size);
        // If the next block exists, set its address to the current block's next pointer
        if (i + 1 < num_blocks) {
            pool->blocks[i].next = &pool->blocks[i + 1];
        }
        // If this is the last block, there is no next block, so set next to NULL
        else {
            pool->blocks[i].next = NULL;
        }
    }
    // Initialize the top of the free stack, block size, and number of blocks
    pool->free_stack = &pool->blocks[0];
    pool->block_size = block_size;
    pool->num_blocks = num_blocks;
    
}

void* pool_alloc(MemoryPool *pool) {
    // Return NULL if the pool is invalid or there are no free blocks
    if (!pool || !pool->free_stack) {
        return NULL;
    }
    // Pop a free block from the stack
    Block *blk = pool->free_stack;
    // Update the top of the free stack
    pool->free_stack = blk->next;
    // Return the pointer to the usable data area of the block
    return blk->data;
}

void pool_free(MemoryPool *pool, void *ptr) {
    size_t offset;
    size_t index;
    // Check for invalid arguments (pool, blocks, or pointer)
    if (!pool || !pool->blocks || !ptr) {
        return;
    }
    char *start = (char *)pool->memory;
    char *end = start + pool->num_blocks * pool->block_size;
    char *pointer = (char *)ptr;
    // Ignore if the pointer is outside the pool memory range
    if (pointer < start || pointer >= end) {
        return;
    }

    offset = pointer - start;
    // nvalid if not aligned with block start (must be a multiple of block_size)
    if (offset % pool->block_size != 0) {
        return;
    }

    // Calculate the index of the block to be freed
    index = offset / pool->block_size;
    // Push the block back to the top of the free stack
    Block *blk = &pool->blocks[index];
    blk->next = pool->free_stack;
    pool->free_stack = blk;
}

void pool_destroy(MemoryPool *pool) {
    // Do nothing if the pool is invalid
    if (!pool) {
        return;
    }
    // Free the pool memory and block metadata
    free(pool->memory);
    free(pool->blocks);
    // Reset all fields to their default state
    pool->memory = NULL;
    pool->blocks = NULL;
    pool->free_stack = NULL;
    pool->block_size = 0;
    pool->num_blocks = 0;
}
