#include "scheduler/scheduler.h"
#include "minHeap1/minHeap.h"
#include "Queue/queue.h"
#include "clk.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Global variables
int scheduler_algorithm;
int quantum;
int current_time = 0;
int running_process_id = -1;
int time_process_started_running = 0;
PCB **processes = NULL;
int process_count = 0;
int max_processes = 100; //todo:change it based on the number of processes
void **readyQueue = NULL;
void **finishedQueue = NULL;
FILE *logFile = NULL;
FILE *perfFile = NULL;

// Statistics
double total_runtime = 0;
double total_waiting_time = 0;
double total_wta = 0;
double sum_squared_wta = 0;
Queue readyQueue; // Queue for ready processes
Queue finishedQueue; // Queue for finished processes



// IPC variables (message queues, etc.)
int msgqid; // Message queue ID 
//!this one for receiving the process from the process generator

int msgqid2; // Message queue ID for sending acknowledgment of receiving the process

// Message structure for IPC
typedef struct {
    long mtype;
    //todo:change it based on the sending from process_generator
    int process_id;
    int arrival_time;
    int runtime;
    int priority;
    //  PCB* process; 
} ProcessMessage;

//comparison function for HPF

int comparePriority(void *a, void *b) {
    PCB *processA = (PCB *)a;
    PCB *processB = (PCB *)b;
    if(processA->priority < processB->priority) {
        return -1; // processA has higher priority
    } else if(processA->priority > processB->priority) {
        return 1; // processB has higher priority
    } else {
        if(processA->arrival_time < processB->arrival_time) {
            return -1; // processA arrives first
        } else if(processA->arrival_time > processB->arrival_time) {
            return 1; // processB arrives first
        } else {
            return 1; //equal priority and arrival time so return any one to start
        }
    }
}




// Function to initialize the scheduler
// This function sets up the scheduler with the specified algorithm and quantum time


void initialization(int algorithm, int q) {
    scheduler_algorithm = algorithm;
    quantum = q;

    // Initialize process array
    processes = (PCB **)malloc(max_processes * sizeof(PCB *));
    if (processes == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    // Initialize queues
    if (algorithm == HPF) {
        readyQueue = createMinHeap(max_processes, comparePriority);
    }else if (algorithm==RR) { // RR
        readyQueue = createQueue();
    }
    else{
        //todo: add the minHeap for SRT
    }

    finishedQueue = createQueue();
    
    // Open log file
    logFile = fopen("scheduler.log", "w");
    fprintf(logFile, "#At time x process y state arr w total z remain y wait k\n");
    
    // Set up IPC (message queue) 
    //todo:change it based on the sending from process_generator
    key_t key = ftok("keyfile", 'a');
    msgqid = msgget(key, 0666 | IPC_CREAT);

    
    if (msgqid == -1) {
        perror("Error creating message queue");
        exit(1);
    }

    // Set up message queue for sending acknowledgment
    key_t key2 = ftok("keyfile2", 'b');
    msgqid2 = msgget(key2, 0666 | IPC_CREAT);
    if (msgqid2 == -1) {
        perror("Error creating message queue for acknowledgment");
        exit(1);
    }
    

    printf("Scheduler initialized with algorithm %d\n", algorithm);
    if (algorithm == RR) {
        printf("Quantum: %d\n", quantum);
    }
}

//receive from the process generator
// This function receives a process message from the message queue

void receive_process() {
    ProcessMessage msg;
    //todo:change it based on the sending from prcoess_genrator
    // Try to receive message without blocking
    if (msgrcv(msgqid, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) != -1) {
        // Create PCB for the new process
        
        PCB *newProcess=createPCB(msg.process_id, msg.priority, msg.arrival_time, msg.runtime);

        //store the process in the processes array
        processes[process_count] = newProcess;
        
        // Add to ready queue based on algorithm
        if (scheduler_algorithm == HPF || scheduler_algorithm == SRT) {
            insertMinHeap(readyQueue, newProcess);
        } else {
            enqueue(readyQueue, newProcess);
        }
        
        process_count++;
        
        printf("Process %d received at time %d\n", newProcess->id, current_time);
    }
    else {
        // No message available
        printf("No new process received at time %d\n", current_time);
    }
}

void scheduleProcess() {
    // Check if there are any processes in the ready queue
    if (isEmpty(readyQueue)) {
        printf("No processes to schedule at time %d\n", current_time);
        return;
    }
    
    // Schedule based on the selected algorithm
    switch (scheduler_algorithm) {
        case HPF:
            scheduleHPF();
            break;
        case SRT:
            scheduleSRT();
            break;
        case RR:
            scheduleRR();
            break;
        default:
            printf("Invalid scheduling algorithm\n");
            break;
    }
}

void scheduleHPF() {
    if (running_process_id == -1 && ((MinHeap*)readyQueue)->size > 0) {
        PCB *nextProcess = (PCB *)extractMin(readyQueue);
        start_process(nextProcess->id);
    }
}

void start_process(int process_id)
{
    PCB *pcb = processes[process_id];
    if (pcb == NULL) {
        printf("Process %d not found\n", process_id);
        return;
    }
    pcb->status = RUNNING;
    if(pcb->start_time == -1) {
        pcb->start_time = current_time;
    }
    running_process_id = process_id;
    time_process_started_running = current_time;

    if(pcb->pid!=-1)
    {
        kill(pcb->pid, SIGCONT); //todo:must be caught in the process
        printf("Process %d started at time %d\n", pcb->id, current_time);
    }
    else
    {
        printf("this pid is not valid %d\n", pcb->pid);
    }
}


void cleanup() {
    // Free memory for processes
    for (int i = 0; i < process_count; i++) {
        destroyPCB(processes[i]);
    }
    free(processes);
    
    // Destroy queues
    destroyHeap(readyQueue);
    destroyQueue(finishedQueue);
    
    // Close log file
    fclose(logFile);
    
    // Destroy message queues
    msgctl(msgqid, IPC_RMID, NULL);
    msgctl(msgqid2, IPC_RMID, NULL);
    
    printf("Scheduler cleaned up\n");
}











 



