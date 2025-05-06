#include "memory.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTAL_MEMORY_SIZE 1024
#define LOG_FILE "memory.log"


// Find the highest power of 2 that is less than or equal to x

int highestPowerOf2(int x) {
  if (x <= 0)
    return 1;

  int block_size = 1;
  while (block_size < x) {
    block_size *= 2;
  }
  return block_size;
}


 // Initialize the memory block
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

memory_block_t *allocateMemory(memory_block_t *root, int size) {
  // Base case: null check
  if (root == NULL)
    return NULL;
    
  // If block is already allocated, we can't use it
  if (root->isFree == 0)
    return NULL;

  // If block is too small for the requested size, we can't use it
  int requiredSize = highestPowerOf2(size);
  if (root->size < requiredSize)
    return NULL;

  // If this is a leaf node with exact size, we can use it
  if (!root->left && !root->right && root->size == requiredSize) {
    root->isFree = 0;
    root->realSize = size; // Store the actual requested size
    root->processId = 0; // Initialize process ID (should be set by caller)
    return root;
  }

  // If this is a leaf node but larger than needed, split it
  if (!root->left && !root->right && root->size > requiredSize) {
    int halfSize = root->size / 2;
    
    // Create both children at once
    root->left = initializeMemoryBlock(halfSize, root->start, root->start + halfSize);
    if (root->left == NULL) {
      // Handle memory allocation failure
      return NULL;
    }
    root->left->parent = root;
    
    root->right = initializeMemoryBlock(halfSize, root->start + halfSize, root->end);
    if (root->right == NULL) {
      // Handle memory allocation failure - clean up left child
      free(root->left);
      root->left = NULL;
      return NULL;
    }
    root->right->parent = root;
    
    // Try allocation in the left child first
    memory_block_t *result = allocateMemory(root->left, size);
    if (result != NULL)
      return result;
      
    // If left allocation fails, try right child
    return allocateMemory(root->right, size);
  }

  int leftBest = findBestAvailableBlock(root->left, requiredSize);
  int rightBest = findBestAvailableBlock(root->right, requiredSize);

  if(leftBest <= rightBest) {
    // Try to allocate in left subtree if it exists
    if (root->left != NULL) {
      memory_block_t *left = allocateMemory(root->left, size);
      if (left != NULL)
        return left;
    }
    // Try to allocate in right subtree if it exists
    if (root->right != NULL) {
      memory_block_t *right = allocateMemory(root->right, size);
      if (right != NULL)
      return right;
    }   
  }
  else
  {
    // Try to allocate in right subtree if it exists
    if (root->right != NULL) {
      memory_block_t *right = allocateMemory(root->right, size);
      if (right != NULL)
      return right;
    }   
    if (root->left != NULL) {
      memory_block_t *left = allocateMemory(root->left, size);
      if (left != NULL)
        return left;
    }
  }
  // No suitable block found
  return NULL;
}

int findBestAvailableBlock(memory_block_t *root, int size) {
  // Base case: null check
  if (root == NULL)
    return TOTAL_MEMORY_SIZE;

  if(!root->left && !root->right && root->isFree && root->size >= size) {
    return root->size;
  }
  // Check left and right children
  int leftSize = findBestAvailableBlock(root->left, size);
  int rightSize = findBestAvailableBlock(root->right, size);
  // Return the smaller size
  if (leftSize < rightSize) {
    return leftSize;
  } else {
    return rightSize;
  }
}

void deallocate_memory(memory_block_t *root, pid_t processId) {
  // Base case: null check
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
          
          // If both children are free and are leaf nodes, merge them
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
  if (root->left != NULL)
    deallocate_memory(root->left, processId);
  
  if (root->right != NULL)
    deallocate_memory(root->right, processId);
}


// findMemoryBlock - Find a memory block by address used by printing
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

// findMemoryBlockByProcessId - Find a memory block by process ID
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


void createMemoryLogFile() {
  FILE *logFileptr = fopen(LOG_FILE, "w");
  if (logFileptr == NULL) {
    perror("Can't Create Log File");
    exit(-1);
  }

  printf("Started Logging\n");
  fclose(logFileptr);
}

void fancyPrintTree(memory_block_t *root, int level) {
  if (root == NULL)
    return;

  printf("%s", root->isFree ? ANSI_GREEN : ANSI_RED);

  for (int i = 0; i < level; i++)
    printf("|  ");

  printf("[%d-%d] %s - Process ID: %d\n", root->start, root->end,
         root->isFree ? "Free" : "Allocated", root->processId);

  printf(ANSI_RESET);

  fancyPrintTree(root->left, level + 1);
  fancyPrintTree(root->right, level + 1);
}

void fancyPrintMemoryBar(memory_block_t *root) {
  if (root == NULL)
    return;

  printf(ANSI_BLACK);
  for (int addr = 0; addr < 1024; addr += 8) {
    memory_block_t *block = findMemoryBlock(root, addr);
    if (block != NULL && !block->isFree) {
      switch (block->processId % 6) {
      case 0:
        printf(ANSI_RED_BG);
        break;
      case 1:
        printf(ANSI_GREEN_BG);
        break;
      case 2:
        printf(ANSI_YELLOW_BG);
        break;
      case 3:
        printf(ANSI_BLUE_BG);
        break;
      case 4:
        printf(ANSI_MAGENTA_BG);
        break;
      case 5:
        printf(ANSI_CYAN_BG);
        break;
      default:
        printf(ANSI_RED_BG);
        break;
      }
      printf("%d", block->processId);
    } else {
      printf(ANSI_WHITE_BG " ");
    }
  }
  printf(ANSI_RESET);
  printf("\n");
}

