#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "clk.h"
#include "scheduler.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>
#include "memory.h"


#define MAX_PROCESSES 100
#define MAX_LINE_LENGTH 256

// Structure to hold process data
struct process_data
{
    int id;
    int arrival_time;
    int runtime;
    int priority;
    pid_t pid;
    int completed;
    int memory_size;
    struct process_data* next; // Pointer to the next process in the list
};
typedef struct process_data process_data;

void sigchld_handler(int sig);
int read_processes(const char *filename, process_data process_list[]);
void display_processes(process_data process_list[], int count);
int check_no_more_processes(int processes_sent, int processCount);
void notifySchedulerFinishedProcess(pid_t pid);
void clear_resources(int signum);
void signals_handling();
void arguments_Reader(int argc, char *argv[], int *algorithm_type, int *quantum, int *processCount, process_data process_list[]);
int sending_process(process_data * process, int current_time);
void sending_waiting_proccess (int current_time, int *processes_send);
void sending_arrival_processes(int *next_process_idx, int processCount, int current_time, int *processes_sent);
void there_is_no_processes(int processes_sent);
void clear_resources(int);
int waiting_list_remove(process_data* process);               // Remove a process from the waiting list
void waiting_list_add(process_data* process);                  // Add a process to the list
void log_memory_stats(process_data* process, char* state, int current_time, int start, int end) ;
process_data* get_process_by_pid(pid_t pid);

void fancyPrintTree(memory_block_t *root, int level);
void fancyPrintMemoryBar(memory_block_t *root);