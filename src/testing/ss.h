// buddy_allocator.h
#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>

#define MEMORY_SIZE 1024
#define MIN_BLOCK_SIZE 32  // Smallest allocation unit
#define MAX_LEVELS 6       // log2(1024/32) + 1 = 6 levels

// Memory block structure
typedef struct MemoryBlock {
    int offset;            // Start address of the block
    int size;              // Size of the block
    int is_free;           // 1 if block is free, 0 if allocated
    struct MemoryBlock* next;
} MemoryBlock;

// Memory allocator structure
typedef struct {
    MemoryBlock* free_lists[MAX_LEVELS];  // Array of free block lists for each level
    FILE* log_file;                       // File to log memory operations
    int sem_id;                        // Semaphore ID for synchronization
    MemoryBlock *allocated_blocks;      // List of allocated blocks
    int total_allocated;  // Track total allocated memory
    int total_free;       // Track total free memory
} BuddyAllocator;

// Function prototypes
void initializeAllocator(BuddyAllocator* allocator, char* log_filename);
int allocateMemory(BuddyAllocator* allocator, int pid, int size, int current_time);
void freeMemory(BuddyAllocator* allocator, int pid, int memory_start, int memory_size, int current_time);
void cleanupAllocator(BuddyAllocator* allocator);
void printMemoryBlocks(BuddyAllocator* allocator);
void lockSemaphore(int sem_id);
void unlockSemaphore(int sem_id);
void recursiveMerge(BuddyAllocator* allocator, MemoryBlock* block, int level);
MemoryBlock* splitBlock(BuddyAllocator* allocator, int level);
int findBuddyOffset(int offset, int size);
int findLevel(int size);

#endif