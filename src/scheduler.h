#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "clk.h"
#include <stdio.h>
#include "models/Queue/queue.h"
#include "models/minHeap1/minHeap.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>

// Constants for scheduling algorithms
#define HPF 1
#define SRTN 2
#define RR 3

#define MAX_PROCESSES 100

// Process states
#define READY 0
#define RUNNING 1
#define BLOCKED 2
#define FINISHED 3

struct PCB
{
    int id;
    int arrival_time;
    int runtime;
    int remaining_time;
    int priority;
    pid_t pid;
    int wait_time;
    int start_time;
    int status;
    struct PCB *next;
    int shm_id;
    int *shm_ptr;
    int ending_time;
};

typedef struct PCB PCB; // Forward declaration for PCB

typedef struct
{
    long mtype;
    int process_id;
    int arrival_time;
    int runtime;
    int priority;
    pid_t pid;
    int shm_id;
} ProcessMessage;

typedef struct
{
    long mtype;      // Message type (use 1 for completion messages)
    int process_id;  // ID of completed process
    int finish_time; // Time when process finished
} CompletionMessage;

void initialize(int alg, int q);
void run_scheduler();
void cleanup();

PCB *select_next_process();
void update_process_times();
void handle_finished_process();
void check_arrivals();
void start_process(PCB *process);
void stop_process(PCB *process);
void log_process_state(PCB *process, char *state);
void log_performance_stats();

int compare_priority(void *a, void *b);
int compare_remaining_time(void *a, void *b);
void PCB_remove(PCB *process);
void PCB_add(PCB *process);
int Empty(void **RQ);

#endif /* SCHEDULER_H */