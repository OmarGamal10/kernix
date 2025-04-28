#ifndef SCHEDULER_H
#define SCHEDULER_H

// Include necessary headers
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
#define HPF 1  // Highest Priority First
#define SRTN 2 // Shortest Remaining Time Next
#define RR 3   // Round Robin

#define MAX_PROCESSES 100

// Process states
#define READY 0    // Process is ready to run
#define RUNNING 1  // Process is currently running
#define FINISHED 3 // Process has finished execution


// Process Control Block (PCB) structure
struct PCB {
    int id;                // Process ID
    int arrival_time;      // Time of arrival
    int runtime;           // Total runtime of the process
    int remaining_time;    // Remaining runtime
    int priority;          // Priority of the process
    pid_t pid;             // Process ID in the system
    int wait_time;         // Time spent waiting
    int start_time;        // Time when the process started
    int status;            // Current status of the process
    struct PCB* next;      // Pointer to the next PCB in the list
    int shm_id;            // Shared memory ID
    int *shm_ptr;          // Pointer to shared memory
    int ending_time;       // Time when the process finished

};

typedef struct PCB PCB; // Typedef for easier usage of PCB


// Message structure for process communication
typedef struct {
    long mtype;            // Message type
    int process_id;        // Process ID
    int arrival_time;      // Time of arrival
    int runtime;           // Total runtime
    int priority;          // Priority of the process
    pid_t pid;             // Process ID in the system
    int shm_id;            // Shared memory ID
} ProcessMessage;

// Message structure for completion notifications
typedef struct {
    long mtype;            // Message type (use 1 for completion messages)
    int process_id;        // ID of the completed process
    int finish_time;       // Time when the process finished
} CompletionMessage;

// Function prototypes for scheduler operations
void initialize(int alg, int q); // Initialize the scheduler with algorithm and quantum
void run_scheduler();           // Main function to run the scheduler
void cleanup();                 // Cleanup resources after scheduling

// Function prototypes for process management
PCB* select_next_process();     // Select the next process to run
void update_process_times();    // Update process times (e.g., remaining time)
void handle_finished_process(); // Handle processes that have finished execution
void check_arrivals();          // Check for newly arrived processes
void start_process(PCB* process); // Start a process
void stop_process(PCB* process);  // Stop a running process
void log_process_state(PCB* process, char* state); // Log the state of a process
void log_performance_stats();   // Log overall performance statistics

// Utility functions for process comparison and management
int compare_priority(void* a, void* b);       // Compare processes by priority
int compare_remaining_time(void* a, void* b); // Compare processes by remaining time
void PCB_remove(PCB* process);               // Remove a process from the list
void PCB_add(PCB* process);                  // Add a process to the list
int Empty(void* RQ);                        // Check if the ready queue is empty


#endif /* SCHEDULER_H */