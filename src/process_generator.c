#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include <sys/wait.h>
#include "clk.h"
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PROCESSES 100
#define MAX_LINE_LENGTH 256

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
    pid_t pid;
} process_data;

int process_count = 0; // Number of processes in the list

// global variables for cleanup
int msgq_id = -1;
pid_t scheduler_pid = -1;

process_data process_list[MAX_PROCESSES];
int main(int argc, char * argv[])
{
    signal(SIGINT, clear_resources);
    
    // Fork the clock process (parent is clock, child is generator)
    pid_t clk_pid = fork();
    
    if (clk_pid == -1) {
        perror("Error forking clock process");
        exit(1);
    }
    
    if (clk_pid == 0)  // Child process (clk)
    {
        init_clk();
        sync_clk();  
        run_clk();        
    }
    else   // Parent process (generator)
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

         printf("argcs count: %d\n", argc);
         
         if (argc > 1) {
             algorithm = atoi(argv[1]);
             if (algorithm == 3 && argc > 2) { // RR needs quantum
                 quantum = atoi(argv[2]);
                 if(argc>3)
                 {
                    printf("test file: %s\n", argv[3]);
                    process_count=read_processes(argv[3],process_list);
                    display_processes(process_list, process_count);
                 }
             }
             else if((algorithm==1||algorithm==2) && argc>2)
             {
                printf("test file: %s\n", argv[2]);
                process_count=read_processes(argv[2],process_list);
                display_processes(process_list, process_count);
             }
             else
             {
                printf("Error: Invalid arguments\n");
                exit(1);
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

         int next_process_idx = 0;
         int current_time = -1;  // Initialize to ensure first tick is processed
         
         // Wait for clock to reach 0 before starting
        while (get_clk() < 0) {
        }
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
                 while (next_process_idx < process_count && 
                        process_list[next_process_idx].arrival_time <= current_time) {
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
                 
                 if (next_process_idx >= process_count) {
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
    
    return 0;
}


int read_processes(const char* filename, process_data process_list[]) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int process_count = 0;

    // Skip lines starting with '#'
    while (fgets(line, sizeof(line), file)) {
        // Skip comment lines
        if (line[0] == '#') {
            continue;
        }

        // Parse the process information
        if (sscanf(line, "%d\t%d\t%d\t%d", 
                  &process_list[process_count].id, 
                  &process_list[process_count].arrival_time, 
                  &process_list[process_count].runtime, 
                  &process_list[process_count].priority) == 4) {
            
            // Initialize pid to 0
            process_list[process_count].pid = 0;
            
            process_count++;
            if (process_count >= MAX_PROCESSES) {
                printf("Warning: Maximum process limit (%d) reached. Ignoring remaining processes.\n", MAX_PROCESSES);
                break;
            }
        }
    }

    fclose(file);
    return process_count;
}


void display_processes(process_data process_list[], int count) {
    printf("\nProcess List:\n");
    printf("ID\tArrival\tRuntime\tPriority\tPID\n");
    printf("------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%d\t%d\t%d\t%d\t%d\n", 
               process_list[i].id, 
               process_list[i].arrival_time, 
               process_list[i].runtime, 
               process_list[i].priority,
               process_list[i].pid);
    }
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
    destroy_clk(1);
    
    exit(0);
}