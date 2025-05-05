#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
/**
 * memory_block_t
 * A struct to represent a memory block in the memory
 */
typedef struct memory_block {
    int size;
    int realSize;
    int start;
    int end;
    pid_t processId;
    int isFree;
  
    // tree like pointers
    struct memory_block *parent;
    struct memory_block *left;
    struct memory_block *right;
  } memory_block_t;
  
  // =============================================================================
  // INTERNAL FUNCTIONS
  int highestPowerOf2(int x);
  memory_block_t *initializeMemoryBlock(int size, int start, int end);
  memory_block_t *findMemoryBlock(memory_block_t *root, int addr);
  memory_block_t *findMemoryBlockByProcessId(memory_block_t *root, int processId);
  
  // API
  memory_block_t *create_memory();
  memory_block_t *allocateMemory(memory_block_t *root, int size);
  void deallocate_memory(memory_block_t *root, pid_t processId);
  
  // output functions
  void createMemoryLogFile();