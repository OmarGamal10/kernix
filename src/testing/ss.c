// buddy_allocator.c
#include "ss.h"
#include <sys/types.h>
#include <sys/ipc.h>

// Initialize the buddy allocator
void initializeAllocator(BuddyAllocator *allocator, char *log_filename) {
    for (int i = 0; i < MAX_LEVELS; i++) {
        allocator->free_lists[i] = NULL;
    }

    // Create the initial block (entire memory)
    MemoryBlock *initial_block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    initial_block->offset = 0;
    initial_block->size = MEMORY_SIZE;
    initial_block->is_free = 1;
    initial_block->next = NULL;

    allocator->total_allocated = 0; // New: Initialize total allocated memory
    allocator->total_free = MEMORY_SIZE; // New: Initialize total free memory

    

    // Place it in the appropriate level
    allocator->free_lists[MAX_LEVELS - 1] = initial_block;

    // Initialize allocated blocks list
    allocator->allocated_blocks = NULL; // New: Initialize to empty

    // Open log file
    allocator->log_file = fopen(log_filename, "w");
    if (allocator->log_file == NULL) {
        perror("Failed to open memory log file");
        exit(1);
    }

    // Write header to log file
    fprintf(allocator->log_file, "#At time x allocated y bytes for process z from i to j\n");

    // Create semaphore
    allocator->sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (allocator->sem_id == -1) {
        perror("Failed to create semaphore");
        exit(1);
    }
    // Initialize semaphore
    if (semctl(allocator->sem_id, 0, SETVAL, 1) == -1) {
        perror("Failed to initialize semaphore");
        exit(1);
    }
}

void lockSemaphore(int sem_id)
{
    struct sembuf sb = {0, -1, 0}; // Decrement semaphore value
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("Failed to lock semaphore");
        exit(1);
    }
}

void unlockSemaphore(int sem_id)
{
    struct sembuf sb = {0, 1, 0}; // Increment semaphore value
    if (semop(sem_id, &sb, 1) == -1)
    {
        perror("Failed to unlock semaphore");
        exit(1);
    }
}

// Find the smallest level that can fit the requested size
int findLevel(int size)
{
    int block_size = MIN_BLOCK_SIZE;
    int level = 0;

    while (block_size < size && level < MAX_LEVELS - 1)
    {
        block_size *= 2;
        level++;
    }

    return level;
}

// Split a block from higher level to get a block at the target level
MemoryBlock *splitBlock(BuddyAllocator *allocator, int target_level)
{
    int current_level = target_level + 1;

    // Find the smallest level with available blocks
    while (current_level < MAX_LEVELS && allocator->free_lists[current_level] == NULL)
    {
        current_level++;
    }

    // If no suitable block found
    if (current_level >= MAX_LEVELS)
    {
        return NULL;
    }

    // Remove a block from the found level
    MemoryBlock *block = allocator->free_lists[current_level];
    allocator->free_lists[current_level] = block->next;

    block->next = NULL;
    // printMemoryBlocks(block); // New: Print the block being split

    // Split the block down to the target level
    while (current_level > target_level)
    {
        int block_size = MIN_BLOCK_SIZE << current_level;
        current_level--;

        // Create a buddy block
        MemoryBlock *buddy = (MemoryBlock *)malloc(sizeof(MemoryBlock));
        buddy->size = block_size / 2;
        buddy->offset = block->offset + buddy->size;
        buddy->is_free = 1;
        buddy->next = allocator->free_lists[current_level];
        allocator->free_lists[current_level] = buddy;

        // Adjust the original block
        block->size = block_size / 2;

    }

    return block;
}

// Allocate memory for a process
int allocateMemory(BuddyAllocator *allocator, int pid, int size, int current_time) {
    if (allocator->total_free < size) {
        return -1; // Not enough memory available
    }
    lockSemaphore(allocator->sem_id); // Lock the semaphore

    // Find the appropriate level for the requested size
    int level = findLevel(size);
    MemoryBlock *block = NULL;

    // Check if there's a free block at this level
    if (allocator->free_lists[level] != NULL) {
        block = allocator->free_lists[level];
        allocator->free_lists[level] = block->next;
        block->next = NULL;
    } else {
        // Split a larger block to get one of the required size
        block = splitBlock(allocator, level);
        if (block == NULL) {
            unlockSemaphore(allocator->sem_id);
            return -1; // Memory allocation failed
        }
    }

    // Mark block as allocated
    block->is_free = 0;

    allocator->total_allocated += block->size;
    allocator->total_free -= block->size;

    // Add to allocated blocks list
    block->next = allocator->allocated_blocks; // New: Insert at head
    allocator->allocated_blocks = block;

    // Log the allocation with the actual block size
    fprintf(allocator->log_file, "At time %d allocated %d bytes for process %d from %d to %d\n",
            current_time, block->size, pid, block->offset, block->offset + block->size - 1);

    unlockSemaphore(allocator->sem_id); // Unlock the semaphore
    printf("Allocated %d bytes at offset %d\n", block->size, block->offset); // New: Print allocation info
    return block->offset;
}

// Find the buddy block address
int findBuddyOffset(int offset, int size)
{
    return offset ^ size;
}

// Free memory for a process
void freeMemory(BuddyAllocator *allocator, int pid, int memory_start, int memory_size, int current_time) {
    lockSemaphore(allocator->sem_id); // Lock the semaphore

    // Find the block in allocated_blocks
    MemoryBlock *prev = NULL;
    MemoryBlock *block = allocator->allocated_blocks;
    while (block != NULL) {
        if (block->offset == memory_start) {
            break;
        }
        prev = block;
        block = block->next;
    }

    // Validate block
    if (block == NULL) {
        fprintf(stderr, "Error: Attempt to free invalid offset %d\n", memory_start);
        unlockSemaphore(allocator->sem_id);
        return;
    }

    // Remove block from allocated_blocks
    if (prev == NULL) {
        allocator->allocated_blocks = block->next;
    } else {
        prev->next = block->next;
    }
    block->next = NULL; // Clear next pointer
    
    int level = findLevel(block->size);
    block->is_free = 1;

    // Log the deallocation with the actual block size
    fprintf(allocator->log_file, "At time %d freed %d bytes from process %d from %d to %d\n",
            current_time, block->size, pid, block->offset, block->offset + block->size - 1);

    // Update total allocated and free memory
    allocator->total_allocated -= block->size;
    allocator->total_free += block->size;

    // Try recursive merging
    recursiveMerge(allocator, block, level);

    unlockSemaphore(allocator->sem_id); // Unlock the semaphore
}

void printMemoryBlocks(BuddyAllocator *allocator) {
    printf("=== MEMORY ALLOCATION STATUS ===\n");
    
    // Print allocated blocks
    printf("Allocated blocks:\n");
    int counter = 1;
    MemoryBlock *block = allocator->allocated_blocks;
    while (block != NULL) {
        printf("Block %d: offset=%d, size=%d, is_free=%d\n", 
               counter++, block->offset, block->size, block->is_free);
        block = block->next;
    }
    
    // Print free blocks by level
    printf("\nFree blocks by level:\n");
    for (int i = 0; i < MAX_LEVELS; i++) {
        printf("Level %d (size %d):\n", i, MIN_BLOCK_SIZE << i);
        counter = 1;
        block = allocator->free_lists[i];
        while (block != NULL) {
            printf("  Block %d: offset=%d, size=%d, is_free=%d\n", 
                   counter++, block->offset, block->size, block->is_free);
            block = block->next;
        }
    }
    
    // Print memory statistics
    printf("\nMemory statistics:\n");
    printf("Total allocated memory: %d bytes\n", allocator->total_allocated);
    printf("Total free memory: %d bytes\n", allocator->total_free);
    printf("Total memory: %d bytes\n", MEMORY_SIZE);
    printf("Memory utilization: %.2f%%\n", 
           (float)allocator->total_allocated / MEMORY_SIZE * 100);
    printf("===============================\n\n");
}

// Helper function for recursive merging
void recursiveMerge(BuddyAllocator *allocator, MemoryBlock *block, int level) {
    // Base case: reached the top level
    if (level >= MAX_LEVELS - 1) {
        block->next = allocator->free_lists[level];
        allocator->free_lists[level] = block;
        return;
    }
    
    // Find buddy offset
    int buddy_offset = findBuddyOffset(block->offset, block->size);
    
    // Search for buddy in free list
    MemoryBlock *prev = NULL;
    MemoryBlock *current = allocator->free_lists[level];
    
    while (current != NULL) {
        if (current->offset == buddy_offset && current->is_free) {
            // Remove buddy from free list
            if (prev == NULL) {
                allocator->free_lists[level] = current->next;
            } else {
                prev->next = current->next;
            }
            
            // Create merged block
            MemoryBlock *merged = (MemoryBlock *)malloc(sizeof(MemoryBlock));
            merged->offset = (block->offset < buddy_offset) ? block->offset : buddy_offset;
            merged->size = block->size * 2;
            merged->is_free = 1;
            merged->next = NULL;
            
            // Free individual blocks
            free(block);
            free(current);
            
            // Try to merge at the next level
            recursiveMerge(allocator, merged, level + 1);
            return;
        }
        prev = current;
        current = current->next;
    }
    
    // No buddy found, add block to current level
    block->next = allocator->free_lists[level];
    allocator->free_lists[level] = block;
}


// Clean up the allocator and close the log file
void cleanupAllocator(BuddyAllocator *allocator) {
    // Free allocated blocks
    MemoryBlock *alloc_block = allocator->allocated_blocks;
    while (alloc_block != NULL) {
        MemoryBlock *next = alloc_block->next;
        free(alloc_block);
        alloc_block = next;
    }
    allocator->allocated_blocks = NULL;

    // Free blocks in free lists
    for (int i = 0; i < MAX_LEVELS; i++) {
        MemoryBlock *block = allocator->free_lists[i];
        while (block != NULL) {
            MemoryBlock *next = block->next;
            free(block);
            block = next;
        }
        allocator->free_lists[i] = NULL;
    }

    // Close the log file
    if (allocator->log_file != NULL) {
        fclose(allocator->log_file);
        allocator->log_file = NULL;
    }

    // Remove semaphore
    if (semctl(allocator->sem_id, 0, IPC_RMID) == -1) {
        perror("Failed to remove semaphore");
    }
}

int main()
{
    return 0;
}