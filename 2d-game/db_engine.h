#ifndef DB_ENGINE_H
#define DB_ENGINE_H
#define MAX_NAME_LEN 31

#include <string.h>
#include <stdlib.h> 

typedef struct Block {
    struct Block *next;
    void *data;        // Pointer to the usable memory area
} Block;

typedef struct {
    void *memory;      // Memory area for the entire pool
    Block *blocks;     // Array of block metadata
    Block *free_stack; // Stack of free blocks
    size_t block_size; // Size of each block
    size_t num_blocks; // Total number of blocks
} MemoryPool;

// Initialize the memory pool
// num_blocks: number of blocks、block_size: size of each block (in bytes)
void pool_init(MemoryPool *pool, size_t num_blocks, size_t block_size);

// Allocate a block from the pool
// Returns a pointer to the block on success, or NULL if the pool is full
void* pool_alloc(MemoryPool *pool);

// Return a block to the pool  
// ptr: a pointer obtained from pool_alloc
void pool_free(MemoryPool *pool, void *ptr);

// Destroy the memory pool and free all allocated memory
void pool_destroy(MemoryPool *pool);


#endif
