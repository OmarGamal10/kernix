#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include <sys/wait.h>
#include "clk.h"
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    long mtype;
    int process_id;      // Regular ID, -1 for no process this tick, -2 for termination
    int arrival_time;
    int runtime;
    int priority;
} ProcessMessage;

void clear_resources(int);

typedef struct {
    int id;
    int arrival_time;
    int runtime;
    int priority;
    int pid;
} process_data;

// global variables for cleanup
int msgq_id = -1;
pid_t scheduler_pid = -1;

int main(int argc, char * argv[])
{
    signal(SIGINT, clear_resources);
    
    // Fork the clock process (parent is clock, child is generator)
    pid_t clk_pid = fork();
    
    if (clk_pid == -1) {
        perror("Error forking clock process");
        exit(1);
    }
    
    if (clk_pid == 0)  // Child process (generator)
    {
        // msg queue setup
        key_t key = ftok("keyfile", 'a');
        if (key == -1) {
            perror("Error in ftok");
            exit(1);
        }
        
        msgq_id = msgget(key, 0666 | IPC_CREAT);
        if (msgq_id == -1) {
            perror("Error in msgget");
            exit(1);
        }
        
        // Get scheduling algorithm (default to HPF if not specified) // for testing
        int algorithm = 1;  // Default: HPF
        int quantum = 1;    // Default quantum for RR
        
        if (argc > 1) {
            algorithm = atoi(argv[1]);
            if (algorithm == 3 && argc > 2) { // RR needs quantum
                quantum = atoi(argv[2]);
            }
        }
        
        // Fork and execute the scheduler
        scheduler_pid = fork();
        if (scheduler_pid == -1) {
            perror("Error forking scheduler");
            exit(1);
        }
        
        if (scheduler_pid == 0) {
            // Scheduler process
            char alg_str[2];
            sprintf(alg_str, "%d", algorithm);
            
            if (algorithm == 3) { // Round Robin
                char quantum_str[10];
                sprintf(quantum_str, "%d", quantum);
                // the actual scheduler process
                execl("./bin/scheduler", "scheduler", alg_str, quantum_str, NULL);
            } else {
                execl("./bin/scheduler", "scheduler", alg_str, NULL);
            }
            
            perror("Error executing scheduler");
            exit(1);
        }
        
    
        
        sync_clk();
        
        // Read processes from the process list (hardcoded for now)
        process_data process_list[] = {
            {1, 0, 5, 1, 0},
            {2, 0, 3, 2, 0},
            {3, 0, 4, 3, 0},
            {4, 3, 2, 4, 0},
            {5, 4, 1, 5, 0}
        };
        int num_processes = sizeof(process_list) / sizeof(process_list[0]);
        int next_process_idx = 0;
        int current_time = -1;  // Initialize to ensure first tick is processed
        
        // Main loop - send a message on every clock tick
        while (1) {
            int new_time = get_clk();
            
            // Only process once per tick
            if (new_time > current_time) {
                current_time = new_time;
                printf("Generator tick at time %d\n", current_time);
                
                ProcessMessage msg;
                msg.mtype = 1;  // Any positive number
                int processes_sent = 0; // Track if any processes were sent
                
                // Check if we have a process arriving at this time
                while (next_process_idx < num_processes && 
                       process_list[next_process_idx].arrival_time == current_time) {
                    // Process arrives at this tick
                    msg.process_id = process_list[next_process_idx].id;
                    msg.arrival_time = process_list[next_process_idx].arrival_time;
                    msg.runtime = process_list[next_process_idx].runtime;
                    msg.priority = process_list[next_process_idx].priority;
                    
                    printf("Sending process %d to scheduler at time %d\n", msg.process_id, current_time);
                    
                    if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("Error sending process message");
                    }
                    
                    next_process_idx++;
                    processes_sent++;
                }
                
                if (next_process_idx >= num_processes) {
                    // All processes have been sent, signal completion
                    msg.process_id = -2;  // Termination signal
                    msg.arrival_time = 0;
                    msg.runtime = 0;
                    msg.priority = 0;
                    
                    printf("All processes sent, sending termination signal\n");
                    
                    if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("Error sending termination message");
                    }
                    
                    break;  // Exit the loop
                }
                // No need to send -1 if processes were sent
                // Only send -1 if no processes arrived this tick
                else if (processes_sent == 0) {
                    msg.process_id = -1;  // No process this tick
                    msg.arrival_time = 0;
                    msg.runtime = 0;
                    msg.priority = 0;
                    
                    printf("No processes at time %d, sending -1\n", current_time);
                    
                    if (msgsnd(msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("Error sending no-process message");
                    }
                }
            }
            
            usleep(1000);
        }
        
        int status;
        waitpid(scheduler_pid, &status, 0);

        clear_resources(0);
    }
    else  
    {
        init_clk();
        sync_clk();  
        run_clk();  
    }
    
    return 0;
}

void clear_resources(int signum)
{
    printf("Cleaning up resources...\n");
    
    // Remove message queue if it exists
    if (msgq_id != -1) {
        msgctl(msgq_id, IPC_RMID, NULL);
    }
    printf("Message queue removed\n");
    // Kill scheduler if it exists
    if (scheduler_pid > 0) {
        kill(scheduler_pid, SIGINT);
    }
    printf("after removeing scheduler\n");

    
    // Clean up clock resources
    destroy_clk(0);
    
    exit(0);
}