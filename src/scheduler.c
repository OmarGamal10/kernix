#include "scheduler.h"
#include <errno.h> // Ensure this is included


int algorithm;          
int quantum;             
int msgq_id;             
FILE* logFile;          
void* readyQueue;        
PCB** processes;         
int process_count = 0;   
int max_processes = 100; //should perhaps be a parameter
PCB* running_process = NULL; 
int current_time = -1;   
int time_slice = 0;      
int terminated = 0;    
bool process_not_arrived = true; 


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <algorithm> [quantum]\n", argv[0]);
        fprintf(stderr, "  algorithm: 1=HPF, 2=SRTN, 3=RR\n");
        fprintf(stderr, "  quantum: required for RR algorithm\n");
        exit(1);
    }
    
    algorithm = atoi(argv[1]);
    
    if (algorithm < 1 || algorithm > 3) {
        fprintf(stderr, "Invalid algorithm. Must be 1 (HPF), 2 (SRTN), or 3 (RR)\n");
        exit(1);
    }
    
    quantum = 0;
    if (algorithm == RR) {
        if (argc < 3) {
            fprintf(stderr, "Error: RR algorithm requires quantum parameter\n");
            exit(1);
        }
        quantum = atoi(argv[2]);
        if (quantum <= 0) {
            fprintf(stderr, "Error: Quantum must be a positive integer\n");
            exit(1);
        }
    }
    
    initialize(algorithm, quantum);
    
    run_scheduler();
    
    cleanup();
    
    return 0;
}

void initialize(int alg, int q) {
    algorithm = alg;
    quantum = q;
    
    signal(SIGINT, (void (*)(int))cleanup);
    
    switch(algorithm) {
        case HPF:
            readyQueue = createMinHeap(max_processes, compare_priority);
            printf("Scheduler started with Highest Priority First algorithm\n");
            break;
        case SRTN:
            readyQueue = createMinHeap(max_processes, compare_remaining_time);
            printf("Scheduler started with Shortest Remaining Time Next algorithm\n");
            break;
        case RR:
            readyQueue = createQueue();
            printf("Scheduler started with Round Robin algorithm, quantum = %d\n", quantum);
            break;
    }
    
    processes = (PCB**)malloc(max_processes * sizeof(PCB*));
    if (!processes) {
        perror("Failed to allocate memory for process array");
        exit(1);
    }
    
    // Create log file (placeholder, i don't use it yet)
    logFile = fopen("scheduler.log", "w");
    if (!logFile) {
        perror("Failed to open log file");
        exit(1);
    }
    fprintf(logFile, "#At time x process y state arr w total z remain y wait k\n");
    

    key_t key = ftok("keyfile", 'a');
    if (key == -1) {
        perror("Error in ftok");
        exit(1);
    }
    
    msgq_id = msgget(key, 0666); // this should be read-only for the scheduler
    if (msgq_id == -1) {
        perror("Error accessing message queue");
        exit(1);
    }
    
    sync_clk();
    //current_time = get_clk();
    
    printf("Scheduler initialized successfully\n");
}

// Main scheduler loop
void run_scheduler() {
    while (!terminated) {
        int new_time = get_clk();
        if (new_time > current_time) {
            current_time = new_time;
            
            check_arrivals();
            
            // Update times and check for finished processes
            if (running_process) {
                running_process->remaining_time--;
                
                if (algorithm == RR) {
                    time_slice++;
                }
                
                // Check if process finished
                if (running_process->remaining_time <= 0) {
                    handle_finished_process();
                }
            }
            
            // Select next process if needed
            PCB* next_process = select_next_process();
            
            if (next_process != running_process) {
                if (running_process && running_process->remaining_time > 0) {
                    stop_process(running_process);
                }
                running_process = next_process;
                if (running_process) {
                    start_process(running_process);
                }
            }
        }
    }
}

// Check for newly arrived processes from the message queue

void check_arrivals() {
    ProcessMessage msg;
    int processes_received = 0; // Track processes received this tick

    // Only check messages if we expect new processes
    if (process_not_arrived) {
        while (true) {
            // Blocking msgrcv to wait for a message
            ssize_t received = msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), 0, 0);
            if (received == -1) {
                perror("Error receiving message");
                return;
            }

            if (msg.process_id == -2) {
                printf("Received no more processes signal at time %d\n", current_time);
                process_not_arrived = false;
                if (running_process == NULL && Empty(readyQueue)) {
                    terminated = true;
                }
                return; // Done for this tick
            }
            else if (msg.process_id == -1) {
                // No process this tick, only exit if no processes were received
                if (processes_received == 0) {
                    printf("No processes at time %d\n", current_time);
                    return;
                }
                // Otherwise, continue checking for more process messages
                continue;
            }

            // Handle new process
            PCB* new_process = (PCB*)malloc(sizeof(PCB));
            if (!new_process) {
                perror("Failed to allocate memory for PCB");
                continue;
            }

            new_process->id = msg.process_id;
            new_process->arrival_time = msg.arrival_time;
            new_process->runtime = msg.runtime;
            new_process->remaining_time = msg.runtime;
            new_process->priority = msg.priority;
            new_process->pid = -1;
            new_process->wait_time = 0;
            new_process->start_time = -1;
            new_process->status = READY;

            // Add to processes array
            if (process_count < max_processes) {
                processes[process_count++] = new_process;
            } else {
                fprintf(stderr, "Max processes exceeded\n");
                free(new_process);
                continue;
            }

            // Add to ready queue
            switch (algorithm) {
                case HPF:
                    insertMinHeap(readyQueue, new_process);
                    break;
                case SRTN:
                    insertMinHeap(readyQueue, new_process);
                    break;
                case RR:
                    enqueue(readyQueue, new_process);
                    break;
            }

            printf("Process %d arrived at time %d\n", new_process->id, current_time);
            log_process_state(new_process, "arrived");
            processes_received++;

            // Peek at the next message non-blocking
            received = msgrcv(msgq_id, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT);
            if (received == -1) {
                if (errno == ENOMSG) {
                    // No more messages, exit
                    return;
                } else {
                    perror("Error peeking at message");
                    return;
                }
            }

            // Re-send the peeked message
            if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                perror("Error re-sending message");
                return;
            }

            // If the next message is not a process message, exit
            if (msg.process_id <= 0) {
                return;
            }
        }
    }

    // Update termination condition
    if (!process_not_arrived && running_process == NULL && Empty(readyQueue)) {
        terminated = true;
    }
}

// Update times for the running process
void update_process_times() {
    if (running_process) {
        running_process->remaining_time--;
        
        if (algorithm == RR) {
            time_slice++;
        }
    }
}

void handle_finished_process() {
    if (running_process && running_process->remaining_time <= 0) {
        printf("Process %d completed at time %d\n", running_process->id, current_time);
        log_process_state(running_process, "finished");
        
        if (running_process->pid > 0) {
            int status;
            waitpid(running_process->pid, &status, 0);
        }
        
        running_process->status = FINISHED;
        
        running_process = NULL;
        time_slice = 0;
    }
}

int compare_priority(void* a, void* b) {
    PCB* p1 = (PCB*)a;
    PCB* p2 = (PCB*)b;
    
    // Lower priority number = higher priority
    if (p1->priority != p2->priority) {
        return p1->priority - p2->priority;
    }
    
    // If priorities are equal, use arrival time as tiebreaker
    return p1->arrival_time - p2->arrival_time;
}

int compare_remaining_time(void* a, void* b) {
    PCB* p1 = (PCB*)a;
    PCB* p2 = (PCB*)b;
    
    // Lower remaining time = higher priority
    if (p1->remaining_time != p2->remaining_time) {
        return p1->remaining_time - p2->remaining_time;
    }
    
    // If remaining times are equal, use arrival time as tiebreaker
    return p1->arrival_time - p2->arrival_time;
}

// Select the next process to run based on the algorithm
// Select the next process to run based on the algorithm
// Select the next process to run based on the algorithm
PCB* select_next_process() {
    // Check if running process is finished first
    if (running_process && running_process->remaining_time <= 0) {
        // Process is finished, don't enqueue it
        return NULL;
    }

    // If using RR and quantum expired, put process back in queue
    if (algorithm == RR && running_process && time_slice >= quantum) {
        printf("Quantum expired for Process %d (remaining time: %d)\n", 
               running_process->id, running_process->remaining_time);
        
        // Only enqueue if process still has remaining time
        if (running_process->remaining_time > 0) {
            enqueue(readyQueue, running_process);
            running_process->status = READY;
        }
        
        PCB* next = NULL;
        running_process = NULL;
        time_slice = 0;
        
        // Get next process from queue if available
        if (!Empty(readyQueue)) {
            next = (PCB *)dequeue(readyQueue);
        }
        
        return next;
    }

    if (running_process) {
        switch(algorithm) {
            case HPF:
                // HPF is non-preemptive, continue with current process
                return running_process;
                
            case SRTN:
                if (!Empty(readyQueue)){
                    PCB* top = getMin(readyQueue);
                    if (top && top->remaining_time < running_process->remaining_time) {
                        // Preempt current process
                        insertMinHeap(readyQueue, running_process);
                        return (PCB *)extractMin(readyQueue);
                    }
                }
                return running_process;
                
            case RR:
                return running_process;
                
            default:
                return running_process;
        }
    }
    
    // No running process, get one from the ready queue
    switch(algorithm) {
        case HPF:
        case SRTN:
            if (!Empty(readyQueue)){
                return (PCB *)extractMin(readyQueue);
            }
            break;
                
        case RR:
            if (!Empty(readyQueue)) {
                return (PCB *)dequeue(readyQueue);
            }
            break;
    }
    
    // No process to run
    return NULL;
}


void start_process(PCB* process) {
    if (!process) return;
    
    process->status = RUNNING;
    
    if (process->start_time == -1) {
        // First time starting this process
        process->start_time = current_time;
        process->wait_time = current_time - process->arrival_time;
        
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork process");
            exit(1);
        }
        else if (pid == 0) {
            // Child process
            char runtime_str[20];
            char id_str[20];
            sprintf(runtime_str, "%d", process->runtime);
            sprintf(id_str, "%d", process->id);
            execl("./bin/process", "process", runtime_str, id_str, NULL);
            perror("Failed to exec process");
            exit(1);
        }
        else {
            process->pid = pid;
            log_process_state(process, "started");
        }
    }
    else {
        kill(process->pid, SIGCONT);
        process->status = RUNNING;

        log_process_state(process, "resumed");
    }
    
    time_slice = 0; 
}

// Stop a process
void stop_process(PCB* process) {
    if (!process) return;
    
    if (process->status == RUNNING && process->remaining_time > 0) {
        // Only stop if process is running and not finished
        printf("Stopping process %d (remaining time: %d)\n", process->id, process->remaining_time);
        
        kill(process->pid, SIGSTOP);
        process->status = READY;
        log_process_state(process, "stopped");
    }
}

// Log process state changes
void log_process_state(PCB* process, char* state) {
    if (!process || !logFile) return;
    
    fprintf(logFile, "At time %d process %d %s arr %d total %d remain %d wait %d\n",
            current_time, process->id, state, process->arrival_time,
            process->runtime, process->remaining_time, process->wait_time);
    fflush(logFile);
}

void cleanup() {
    printf("Cleaning up scheduler resources...\n");
    
    if (logFile) {
        fclose(logFile);
    }
    
    if (processes) {
        for (int i = 0; i < process_count; i++) {
            if (processes[i]) {
                // If process is running, kill it
                if (processes[i]->pid > 0 && processes[i]->status != FINISHED) {
                    kill(processes[i]->pid, SIGKILL);
                }
                free(processes[i]);
            }
        }
        free(processes);
    }
    
    // Free ready queue
    switch(algorithm) {
        case HPF:
        case SRTN:
            if (readyQueue) destroyHeap(readyQueue);
            break;
        case RR:
            if (readyQueue) free(readyQueue);
            break;
    }
    
    destroy_clk(0);
    
    exit(0);
}

int Empty(void ** RQ){
    if (algorithm == RR)
    {
        Queue * q = (Queue*)RQ;
        return q->size == 0;
    }
    else{
        MinHeap * mh = (MinHeap*)RQ;
        return mh->size == 0;
    }
}