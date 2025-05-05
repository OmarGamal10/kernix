#ifndef VIRTUAL_MEM
#define VIRTUAL_MEM

#include "memory.h"
typedef struct processQueue {
  int size;
  int pid;
  struct processQueue* next;

} virtualMemoryQueue;

// To add a process to virtual memory queue
int virtualMemoryEnqueue(int pid, int processSize);

// Get process from virtual memory with given pid if exists
Chunk* get_process_virt_mem(int pid);

// To remove the first element of queue if freememory >= front->size
void* virtualMemoryDequeue(int freeMemory);

// To display the memory
void display_virtual_memory();

// To empty the queue
void flushQueue();

#endif