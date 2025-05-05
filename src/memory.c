#include "memory.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_MEMORY_SIZE 1024
#define LOG_FILE "memory.log"

/**
 * highestPowerOf2 - Find the highest power of 2 that is less than or equal to x
 *
 * @param x - The number
 * @return int - The highest power of 2
 */
int highestPowerOf2(int x) {
  if (x <= 0)
    return 1;

  int block_size = 1;
  while (block_size < x) {
    block_size *= 2;
  }
  return block_size;
}

/**
 * initMemory - Initialize the memory block
 *
 * @return memory_block_t* - The memory block
 */
memory_block_t *create_memory() {
  memory_block_t *memory = malloc(sizeof(memory_block_t));
  memory->size = TOTAL_MEMORY_SIZE;
  memory->start = 0;
  memory->end = TOTAL_MEMORY_SIZE;
  memory->realSize = -1;
  memory->left = NULL;
  memory->right = NULL;
  memory->parent = NULL;
  memory->processId = -1;
  memory->isFree = 1;
  return memory;
}

/**
 * initializeMemoryBlock - Initialize a memory block
 *
 * @param size - The size of the memory block
 * @param start - The start address of the memory block
 * @param end - The end address of the memory block
 * @return memory_block_t* - The memory block
 */
memory_block_t *initializeMemoryBlock(int size, int start, int end) {
  memory_block_t *block = malloc(sizeof(memory_block_t));

  if (block != NULL) {
    block->size = highestPowerOf2(size);
    block->realSize = size;
    block->start = start;
    block->end = end;
    block->processId = -1;
    block->isFree = 1;
    block->parent = NULL;
    block->left = NULL;
    block->right = NULL;
  }
  return block;
}

/**
 * allocateMemory - Allocate memory for a process
 *
 * @param root - The root of the memory block
 * @param size - The size of the memory block
 * @param processId - The process ID
 * @return memory_block_t* - The memory block
 */
memory_block_t *allocateMemory(memory_block_t *root, int size) {
  if (root == NULL)
    return NULL;
    
  if (root->isFree == 0)
    return NULL;

  if (root->size < highestPowerOf2(size))
    return NULL;

  if (!root->left && !root->right && root->size == highestPowerOf2(size)) {
    root->isFree = 0;
    return root;
  }

  memory_block_t *left = allocateMemory(root->left, size);
  if (left != NULL)
    return left;

  if (root->size > highestPowerOf2(size)) {
    int leftSize = root->size / 2;

    if (root->left == NULL) {
      root->left =
          initializeMemoryBlock(leftSize, root->start, root->start + leftSize);
      root->left->parent = root;
      return allocateMemory(root->left, size);
    } else if (root->right == NULL) {
      root->right =
          initializeMemoryBlock(leftSize, root->start + leftSize, root->end);
      root->right->parent = root;
      return allocateMemory(root->right, size);
    }
  }

  memory_block_t *right = allocateMemory(root->right, size);
  if (right != NULL)
    return right;
  return NULL;
}

/**
 * freeMemory - Free memory for a process
 *
 * @param root - The root of the memory block
 * @param processId - The process ID
 */
void deallocate_memory(memory_block_t *root, pid_t processId) {
  if (root == NULL)
      return;
  
  // Check if this is the block we want to free
  if (root->processId == processId) {
      // Mark the block as free
      root->isFree = 1;
      root->processId = -1;
      
      // Handle merging of free blocks - we need to go up the tree
      memory_block_t *current = root;
      memory_block_t *parent = root->parent;
      
      // If it has a parent, try to merge siblings
      while (parent != NULL) {
          // Check if sibling exists and is free
          memory_block_t *sibling = (parent->left == current) ? parent->right : parent->left;
          
          // If both children are free, merge them by removing the children
          if (sibling != NULL && sibling->isFree && current->isFree && 
              !current->left && !current->right && !sibling->left && !sibling->right) {
              
              // Free the memory for the children
              free(parent->left);
              free(parent->right);
              parent->left = NULL;
              parent->right = NULL;
              parent->isFree = 1;
              
              // Continue up the tree
              current = parent;
              parent = parent->parent;
          } else {
              // Can't merge further
              break;
          }
      }
      return;
  }
  
  // If not found, search in children
  deallocate_memory(root->left, processId);
  deallocate_memory(root->right, processId);
}


/**
 * findMemoryBlock - Find a memory block by address used by printing
 *
 * @param root - The root of the memory block
 * @param addr - The address
 * @return memory_block_t* - The memory block
 */
memory_block_t *findMemoryBlock(memory_block_t *root, int addr) {
  if (root == NULL)
    return NULL;

  if (!root->left && !root->right && addr >= root->start && addr < root->end)
    return root;

  memory_block_t *left = findMemoryBlock(root->left, addr);
  if (left != NULL)
    return left;

  return findMemoryBlock(root->right, addr);
}

/**
 * findMemoryBlockByProcessId - Find a memory block by process ID
 *
 * @param root - The root of the memory block
 * @param processId - The process ID
 * @return memory_block_t* - The memory block
 */
memory_block_t *findMemoryBlockByProcessId(memory_block_t *root, pid_t processId) {
  if (root == NULL)
    return NULL;

  if (root->processId == processId)
    return root;

  memory_block_t *left = findMemoryBlockByProcessId(root->left, processId);
  if (left != NULL)
    return left;

  return findMemoryBlockByProcessId(root->right, processId);
}

/**
 * createMemoryLogFile - Create the memory log file
 */
void createMemoryLogFile() {
  FILE *logFileptr = fopen(LOG_FILE, "w");
  if (logFileptr == NULL) {
    perror("Can't Create Log File");
    exit(-1);
  }

  printf("Started Logging\n");
  fclose(logFileptr);
}
