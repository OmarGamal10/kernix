#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include <sys/wait.h>
#include "clk.h"
#include "scheduler.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>

#define MAX_PROCESSES 100
#define MAX_LINE_LENGTH 256

void clear_resources(int);

typedef struct {
    int id;
    int arrival_time;
    int runtime;
    int priority;
    pid_t pid;
    int completed;
} process_data;

int processCount = 0; // Number of processes in the list

// global variables for cleanup
int arrG_msgq_id = -1;
int compG_msgq_id = -1;

pid_t scheduler_pid = -1;
void sigchld_handler(int sig) {
    int saved_errno = errno; // Save errno to restore it later
    pid_t pid;
    int status;
    
    // Use waitpid with WNOHANG to collect all terminated children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Check if the child actually exited (not stopped or continued)
        if (WIFEXITED(status)) {
            printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
            // Handle child exit here
            notifySchedulerFinishedProcess(pid);
        }
        else if (WIFSIGNALED(status)) {
            printf("Child %d killed by signal %d\n", pid, WTERMSIG(status));
            // Handle child terminated by signal
        }
        // Ignore other cases (stopped, continued)
    }
    
    errno = saved_errno; // Restore errno
}

process_data process_list[MAX_PROCESSES];
int main(int argc, char * argv[])
{
    signal(SIGINT, clear_resources);
    
    // Fork the clock process (parent is clock, child is generator)
    pid_t clk_pid = fork();
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // SA_NOCLDSTOP is key here
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
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

        key_t arr_key = ftok("keyfile", 'a');
        arrG_msgq_id = msgget(arr_key, 0666 | IPC_CREAT);
        if (arrG_msgq_id == -1) {
            perror("Error creating arrival message queue");
            exit(1);
        }
        
        key_t comp_key = ftok("keyfile", 'c');
        compG_msgq_id = msgget(comp_key, 0666 | IPC_CREAT);
        if (compG_msgq_id == -1) {
            perror("Error creating completion message queue");
            exit(1);
        }
         
         // Get scheduling algorithm (default to HPF if not specified) // for testing
         char* algorithm = "hpf";  // Default: HPF
         int algoritm_type = 0; // 0: HPF, 1: SRTN, 2: RR
         int quantum = 1;    // Default quantum for RR

         printf("argcs count: %d\n", argc);
         
         if (argc > 1) {
             algorithm = argv[1];
             printf("Algorithm: %s\n", algorithm);
             printf(strcmp(algorithm,"rr") == 0 ? "RR algorithm selected\n" : "Not RR algorithm\n");
             printf(strcmp(algorithm,"hpf") == 0 ? "HPF algorithm selected\n" : "Not HPF algorithm\n");

                
             if (strcmp(algorithm,"rr") == 0 && argc > 2) { // RR needs quantum
                 quantum = atoi(argv[2]);
                 algoritm_type = 3; // RR
                 if(argc>3)
                 {
                    printf("test file: %s\n", argv[3]);
                    processCount=read_processes(argv[3],process_list);
                    display_processes(process_list, processCount);
                 }
             }
                else if (strcmp(algorithm,"hpf") == 0) {
                    printf("i entered here");
                    algoritm_type = 1; // HPF
                    printf("test file: %s\n", argv[2]);
                    if(argc>2)
                    {
                        processCount=read_processes(argv[2],process_list);
                        display_processes(process_list, processCount);
                    }
                    else{
                        printf("Error: Invalid arguments\n");
                        exit(1);
                    }
                }
                else if (strcmp(algorithm,"srt") == 0) {
                    algoritm_type = 2; // SRTN
                    if(argc>2)
                    {
                        processCount=read_processes(argv[2],process_list);
                        display_processes(process_list, processCount);
                    }
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
            initialize(algoritm_type, quantum);
            run_scheduler();
            cleanup();
            exit(0); // Exit after cleanup
         }
         
     
         
         sync_clk();

         
         

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

                
                 if (next_process_idx >= processCount) {
                    // All processes have been sent, signal completion
                    msg.process_id = -2;  // Termination signal
                    msg.arrival_time = 0;
                    msg.runtime = 0;
                    msg.priority = 0;
                    
                    printf("All processes sent, sending termination signal\n");
                    
                    if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("Error sending termination message");
                    }
                    
                    break;  // Exit the loop
                }

                while (next_process_idx < processCount && 
                    process_list[next_process_idx].arrival_time <= current_time) {
                
                // Create shared memory for this process
                int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT);
                if (shm_id == -1) {
                    perror("Failed to create shared memory");
                    continue;
                }
                
                int *shm_ptr = (int *)shmat(shm_id, NULL, 0);
                printf("shared memory id %d\n", shm_id);
                printf("shared memory pointer %p\n", (void*)shm_ptr);
                printf("shared memory value %d\n", *shm_ptr);
                if (shm_ptr == (void *)-1) {
                    perror("Failed to attach shared memory");
                    shmctl(shm_id, IPC_RMID, NULL); // Clean up shared memory
                    continue;
                }

                *shm_ptr = process_list[next_process_idx].runtime; // Initialize shared memory with runtime
                printf("shared memory new value %d\n", *shm_ptr);
                // printf("Process %d (PID: %d) created shared memory %d\n", 
                //        process_list[next_process_idx].id, getpid(), *shm_ptr);
                
                // Fork the process
                pid_t process_pid = fork();
                
                if (process_pid == -1) {
                    perror("Failed to fork process");
                    shmdt(shm_ptr);
                    shmctl(shm_id, IPC_RMID, NULL);
                    continue;
                }
                
                if (process_pid == 0) {  // Child process
                    // Execute the process binary
                    char runtime_str[20], id_str[20], shm_id_str[20];
                    sprintf(runtime_str, "%d", process_list[next_process_idx].runtime);
                    sprintf(id_str, "%d", process_list[next_process_idx].id);
                    sprintf(shm_id_str, "%d", shm_id);

                    
                    execl("./bin/process", "process", runtime_str, id_str, shm_id_str, NULL);
                    
                    perror("Failed to execute process");
                    exit(1);
                }

                kill(process_pid, SIGSTOP);
                // Store the process ID in the process list
                process_list[next_process_idx].pid = process_pid;
                printf("Process %d (PID: %d) created and stopped\n", process_list[next_process_idx].id, process_pid);
                
                // Parent process - send message to scheduler
                msg.process_id = process_list[next_process_idx].id;
                msg.arrival_time = process_list[next_process_idx].arrival_time;
                msg.runtime = process_list[next_process_idx].runtime;
                msg.priority = process_list[next_process_idx].priority;
                msg.pid = process_pid;
                msg.shm_id = shm_id;
                
                printf("Forked and sending process %d (PID: %d) to scheduler at time %d\n", 
                        msg.process_id, msg.pid, current_time);
                
                if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                    perror("Error sending process message");
                }
                
                // Detach from shared memory
                shmdt(shm_ptr);
                
                next_process_idx++;
                processes_sent++;
                }
                 

                 // No need to send -1 if processes were sent
                 // Only send -1 if no processes arrived this tick
                if (processes_sent == 0) {
                     msg.process_id = -1;  // No process this tick
                     msg.arrival_time = 0;
                     msg.runtime = 0;
                     msg.priority = 0;
                     
                     if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                         perror("Error sending no-process message");
                     }
                 }
                //  check_completed_processes(process_list, num_processes);
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
// Function to check for completed processes
void check_completed_processes(process_data* process_list, int num_processes) {
    for (int i = 0; i < num_processes; i++) {

        // Skip processes not yet created or already completed
        if (process_list[i].pid <= 0 || process_list[i].completed)
            continue;
            
        int status;
        pid_t result = waitpid(process_list[i].pid, &status, WNOHANG);
        printf(result == 0 ? "Process %d (PID: %d) is still running\n" : "Process %d (PID: %d) has status %d\n", 
               process_list[i].id, process_list[i].pid, result);

        
        if (result > 0) {  // Process has terminated
            // Mark the process as completed
            process_list[i].completed = 1;
            printf("Process %d (PID: %d) completed with status %d\n", 
                   process_list[i].id, process_list[i].pid, WEXITSTATUS(status));
            printf("Process %d (PID: %d) detected as completed\n", 
                   process_list[i].id, process_list[i].pid);
            
            // Send completion message to scheduler via completion queue
            CompletionMessage msg;
            msg.mtype = 1;
            msg.process_id = process_list[i].id;
            msg.finish_time = get_clk();
            
            if (msgsnd(compG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                perror("Error sending completion message");
            } else {
                printf("Sent completion notification for process %d to scheduler\n", 
                       process_list[i].id);
            }
        }
    }
}

void notifySchedulerFinishedProcess(pid_t pid){
    CompletionMessage msg;
    msg.mtype = 1;
    msg.process_id = pid;
    msg.finish_time = get_clk();
    
    if (msgsnd(compG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Error sending completion message");
    } else {
        printf("Sent completion notification for process %d to scheduler\n", 
               pid);
    }
}

void clear_resources(int signum)
{
    static int already_cleaning = 0;
    if (already_cleaning) return;
    already_cleaning = 1;

    printf("Cleaning up resources...\n");
    
    // Remove message queue if it exists
    if (arrG_msgq_id != -1) {
        msgctl(arrG_msgq_id, IPC_RMID, NULL);
    }
    printf("Arrived Message queue removed\n");

    if (compG_msgq_id != -1) {
        msgctl(compG_msgq_id, IPC_RMID, NULL);
    }
    printf("Completed Message queue removed\n");
    // Kill scheduler if it exists
    if (scheduler_pid > 0) {
        kill(scheduler_pid, SIGINT);
    }
    printf("after removeing scheduler\n");

    
    // Clean up clock resources
    destroy_clk(1);
    
    exit(0);
}