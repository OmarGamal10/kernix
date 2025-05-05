#include "buddy_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "memory.h"
#include "virtual_memory.h"

void *mem;
int max_size;
List holes;

void make_mem(int max) {
  max_size = max;
  mem = calloc(max_size, sizeof(Chunk));
  ((Chunk *)mem)->size = max_size;
  ((Chunk *)mem)->pid = -1;
  insert_hole(&holes, max_size, mem);
}
static int alloc(int pid, int size) {
  Chunk *temp = (Chunk *)mem;

  temp = (Chunk *)get_hole(&holes, size);

  // No hole with adequate size found
  if (temp == (Chunk *)-1) {
    return -1;
  }

  // We keep subdividing into two parts, until current size is just adequate for
  // the reuirement.
  while (temp->size / 2 >= size) {
    // Inser hole of half size, which begins at current position+half-size
    insert_hole(&holes, temp->size / 2, (void *)(temp + temp->size / 2));
    // Set allocation pid of the 'other' hole to -1
    (temp + temp->size / 2)->pid = -1;
    // Set size of both holes to half of original size
    temp->size = (temp + temp->size / 2)->size = temp->size / 2;
  }
  // Set allocation pid
  temp->pid = pid;
  return 0;
}

int allocate_page(int pid, int size) {
  int stat = alloc(pid, size);
  if (stat != 0) {
    stat = virtualMemoryEnqueue(pid, size);
  }
  return stat;
}
static int delloc(int pid) {
  Chunk *pos = (Chunk *)mem;

  // Code to find process with given pid in memory
  while (pos < ((Chunk *)mem + max_size)) {
    if (pos->pid == pid) {
      // Found process
      break;
    }
    pos = pos + pos->size;
  }

  if (pos == (Chunk *)mem + max_size) {
    // Process not found
    return -1;
  }

  Chunk *d = (Chunk *)pos;
  d->pid = -1;

  Chunk *temp = (Chunk *)get_adjecent_hole(&holes, d, d->size);

  // Keep combining the adjecent holes until no adjecnt hole of same size
  // remains
  while (temp != (Chunk *)-1) {
    int size = temp->size;
    // Set starting position of the combined hole,whichever of temp and pos that
    // comes first.
    Chunk *loc = temp > d ? d : temp;
    memset(loc, 0, size * 2);
    loc->pid = -1;
    loc->size = size * 2;
    d = loc;
    temp = (Chunk *)get_adjecent_hole(&holes, loc, size * 2);
  }
  // Add final combined hole to list
  insert_hole(&holes, d->size, d);
}

int free_page(int pid) {
  int stat = delloc(pid);

  if (stat != 0) {
    Chunk *temp = get_process_virt_mem(pid);
    if (temp != (Chunk *)-1) {
      free(temp);
      stat = 0;
    } else {
      stat = -1;
    }
  }
  return stat;
}

void *get_process(int pid) {
  Chunk *pos = (Chunk *)mem;

  // Code to find process with given pid in memory
  while (pos < ((Chunk *)mem + max_size)) {
    if (pos->pid == pid) {
      // Found process
      break;
    }
    pos = pos + pos->size;
  }

  // Process found
  if (pos != (Chunk *)mem + max_size) {
    return (void *)pos;
  }

  // Process not in memory,try to reteive from virtual memory
  Chunk *temp = get_process_virt_mem(pid);
  if (temp == (Chunk *)-1) {
    // Process not allocated any page at all
    // i.e, not in virtual memory either
    return (void *)-1;
  }

  // Process was in virtual memory
  // So we keep shifting process from memory to virtual memory
  // until we have enough space to accomodate the process
  while (alloc(temp->pid, temp->size) != 0) {
    Chunk *t = (Chunk *)mem;
    virtualMemoryEnqueue(t->pid, t->size);
    delloc(t->pid);
  }

  // Now we have entered the process in the memory
  // And we find it
  pos = (Chunk *)mem;
  while (pos < ((Chunk *)mem + max_size)) {
    if (pos->pid == pid) {
      // Found process
      break;
    }
    pos = pos + pos->size;
  }
  if (pos == (Chunk *)mem + max_size) {
    return (void *)-1;
  }
  free(temp);
  return pos;
}

void print_stat() {
  printf("Printing current status of Main memory : \n");
  Chunk *temp = (Chunk *)mem;
  int size = 0;
  while (1) {
    printf("%d:%d\t", temp->pid, temp->size);
    size += temp->size;
    if (size >= max_size) break;
    temp = (temp + temp->size);
  }
  printf("\n");
  display_virtual_memory();
}