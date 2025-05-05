#ifndef BUDDY_SYS
#define BUDDY_SYS

//* Init Function for memory. Must Be called before alloc or delloc.
//* Prepares holes list.
void make_mem(int max_size);

//* Function to allocate memory
int allocate_page(int pid, int size);

//* Function to de-allocate memory
int free_page(int pid);

//* Get pointer to process with given pid, loads process in maimory if in
//* virtual memory
void *get_process(int pid);
//* Iterates through memory Chunks and prints status of each chunk.
void print_stat();

#endif