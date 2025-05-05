#include "process_generator.h"


int processCount = 0;
FILE* memoryLogFile; // File pointer for memory log

int arrG_msgq_id = -1; // Message queue ID for arrival messages
int compG_msgq_id = -1; // Message queue ID for completion messages

pid_t scheduler_pid = -1;

process_data* waiting_list_HEAD;
process_data* waiting_list_TAIL;

memory_block_t *memory_root = NULL;

// Signal handler for SIGCHLD to handle terminated child processes
void sigchld_handler(int sig)
{
    int saved_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            notifySchedulerFinishedProcess(pid);
        }
    }

    errno = saved_errno;
}


process_data process_list[MAX_PROCESSES];

int main(int argc, char *argv[])
{
    memory_root = create_memory();
    signals_handling();

    memoryLogFile = fopen("memory.log", "w");
        if (memoryLogFile == NULL)
        {
            perror("Error opening memory log file");
            exit(1);
        }
    fprintf(memoryLogFile, "#At time x allocated y bytes for process z from i to j\n");
    fflush(memoryLogFile);

    pid_t clk_pid = fork();

    if (clk_pid == -1)
    {
        perror("Error forking clock process");
        exit(1);
    }

    if (clk_pid == 0) // Child process (clk)
    {
        // Initialize and run the clock
        init_clk();
        sync_clk();
        run_clk();
    }
    else // Parent process (generator)
    {
        // Create message queues for communication with the scheduler
        key_t arr_key = ftok("keyfile", 'a');
        arrG_msgq_id = msgget(arr_key, 0666 | IPC_CREAT);
        if (arrG_msgq_id == -1)
        {
            perror("Error creating arrival message queue");
            exit(1);
        }

        key_t comp_key = ftok("keyfile", 'c');
        compG_msgq_id = msgget(comp_key, 0666 | IPC_CREAT);
        if (compG_msgq_id == -1)
        {
            perror("Error creating completion message queue");
            exit(1);
        }

        // Parse command-line arguments to determine scheduling algorithm and input file
        int algoritm_type = 0;   // 1: HPF, 2: SRTN, 3: RR
        int quantum = 1;         // Default quantum for RR
        int processCount = 0;
        arguments_Reader(argc, argv, &algoritm_type, &quantum, &processCount, process_list);

        if (processCount == 0)
        {
            printf("No processes to generate\n");
            exit(1);
        }
        
        // Fork and execute the scheduler
        scheduler_pid = fork();
        if (scheduler_pid == -1)
        {
            perror("Error forking scheduler");
            exit(1);
        }

        if (scheduler_pid == 0)
        {
            // Scheduler process
            initialize(algoritm_type, quantum);
            run_scheduler();
            cleanup();
            exit(0);
        }

        sync_clk();
        
        int next_process_idx = 0;
        int current_time = -1;

        waiting_list_HEAD = NULL;
        waiting_list_TAIL = NULL;

        while (get_clk() < 0)
        {
        }
        // Main loop - send a message on every clock tick
        while (1)
        {
            int new_time = get_clk();

            if (new_time > current_time)
            {
                current_time = new_time;
                printf("\033[1;31m");
                printf("[Process Generator] ");
                printf("\033[0m");
                printf("Generator tick at time %d\n", current_time);

                int processes_sent = 0; // Track if any processes were sent

                if (check_no_more_processes(next_process_idx, processCount))
                {
                    break;
                }

                sending_waiting_proccess(current_time, &processes_sent);

                sending_arrival_processes(&next_process_idx, processCount, current_time, &processes_sent);

                there_is_no_processes(processes_sent);

            }
            usleep(1000); // Sleep for 1ms
        }

        // Wait for the scheduler to finish
        int status;
        waitpid(scheduler_pid, &status, 0);

        clear_resources(0); // Clean up resources
    }

    return 0;
}



void signals_handling()
{
    signal(SIGINT, clear_resources);
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
}

void arguments_Reader(int argc, char *argv[], int *algorithm_type, int *quantum, int *processCount, process_data process_list[])
{
    if (argc != 7 && argc != 5)
    {
        printf("Invalid number of arguments\n");
        exit(1);
    }
    if (strcmp(argv[1], "-s") != 0)
    {
        printf("please enter -s before the algorithm\n");
        exit(1);
    }

    char *algorithm = argv[2];

    if (strcmp(algorithm, "rr") == 0 && argc == 7 && strcmp(argv[3], "-q") == 0)
    {
        *quantum = atoi(argv[4]); // Dereference the pointer to update the value in main
        *algorithm_type = 3;      // RR
        if (strcmp(argv[5], "-f") == 0)
        {
            *processCount = read_processes(argv[6], process_list); // Dereference the pointer
            display_processes(process_list, *processCount);
        }
        else
        {
            printf("Error: Invalid arguments\n");
            exit(1);
        }
    }
    else if (strcmp(algorithm, "srtn") == 0 && argc == 5)
    {
        *algorithm_type = 2;
        if (strcmp(argv[3], "-f") == 0)
        {
            *processCount = read_processes(argv[4], process_list); // Dereference the pointer
            display_processes(process_list, *processCount);
        }
        else
        {
            printf("please enter -f before file name\n");
            exit(1);
        }
    }
    else if (strcmp(algorithm, "hpf") == 0 && argc == 5)
    {
        *algorithm_type = 1; // HPF
        if (strcmp(argv[3], "-f") == 0)
        {
            *processCount = read_processes(argv[4], process_list); // Dereference the pointer
            display_processes(process_list, *processCount);
        }
        else
        {
            printf("please enter -f before file name\n");
            exit(1);
        }
    }
    else
    {
        printf("Error: Invalid arguments\n");
        exit(1);
    }
}


int read_processes(const char *filename, process_data process_list[])
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int process_count = 0;

    // Skip lines starting with '#'
    while (fgets(line, sizeof(line), file))
    {
        // Skip comment lines
        if (line[0] == '#')
        {
            continue;
        }

        // Parse the process information
        if (sscanf(line, "%d\t%d\t%d\t%d\t%d",
                   &process_list[process_count].id,
                   &process_list[process_count].arrival_time,
                   &process_list[process_count].runtime,
                   &process_list[process_count].priority,
                   &process_list[process_count].memory_size) == 5)
        {
            process_list[process_count].pid = 0;
            
            process_count++;
            
            if (process_count >= MAX_PROCESSES)
            {
                printf("\033[1;31m");
                printf("[Process Generator] ");
                printf("\033[0m");
                printf("Warning: Maximum process limit (%d) reached. Ignoring remaining processes.\n", MAX_PROCESSES);
                break;
            }
        }
    }
    fclose(file);
    return process_count;
}



// Function to display the list of processes
void display_processes(process_data process_list[], int count)
{
    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("\nProcess List:\n");
    printf("ID\tArrival\tRuntime\tPriority\tPID\tmemsize\n");
    printf("------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
               process_list[i].id,
               process_list[i].arrival_time,
               process_list[i].runtime,
               process_list[i].priority,
               process_list[i].pid,
               process_list[i].memory_size);
    }
}


int check_no_more_processes(int next_process_idx, int processCount)
{
    ProcessMessage msg;
    msg.mtype = 1; // Any positive number

    if (next_process_idx >= processCount && waiting_list_HEAD == NULL)
    {
        // All processes have been sent, signal completion
        msg.process_id = -2; // Termination signal
        msg.arrival_time = 0;
        msg.runtime = 0;
        msg.priority = 0;

        printf("\033[1;31m");
        printf("[Process Generator] ");
        printf("\033[0m");
        printf("All processes sent, sending termination signal\n");

        if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
        {
            perror("Error sending termination message");
        }
        return 1;
    }
    return 0;
}

void sending_waiting_proccess (int current_time, int *processes_send){
    process_data* currentP = waiting_list_HEAD;
    while (currentP != NULL)
    {
        printf("Sending waiting processes at time %d\n", current_time);
        memory_block_t* memory = allocateMemory(memory_root, currentP->memory_size);
        if(memory == NULL)
            break;
        log_memory_stats(currentP, "allocated", current_time, memory->start, memory->end);
        waiting_list_remove(currentP);
        *processes_send += sending_process(currentP, current_time);
        memory->processId = currentP->pid;
        currentP=currentP->next;
    }

}

int sending_process(process_data * process, int current_time){
    ProcessMessage msg;
    // Create shared memory for this process
    int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT);
    if (shm_id == -1)
    {
        perror("Failed to create shared memory");
        return 0;
    }

    int *shm_ptr = (int *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1)
    {
        perror("Failed to attach shared memory");
        shmctl(shm_id, IPC_RMID, NULL); // Clean up shared memory
        return 0;
    }

    *shm_ptr = process->runtime; // Initialize shared memory with runtime

    // Fork the process
    pid_t process_pid = fork();

    if (process_pid == -1)
    {
        perror("Failed to fork process");
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        return 0;
    }

    if (process_pid == 0)
    { // Child process
        char runtime_str[20], id_str[20], shm_id_str[20];
        sprintf(runtime_str, "%d", process->runtime);
        sprintf(id_str, "%d", process->id);
        sprintf(shm_id_str, "%d", shm_id);

        execl("process", "process", runtime_str, id_str, shm_id_str, NULL);

        perror("Failed to execute process");
        exit(1);
    }

    kill(process_pid, SIGSTOP);
    process->pid = process_pid;

    msg.process_id = process->id;
    msg.arrival_time = process->arrival_time;
    msg.runtime = process->runtime;
    msg.priority = process->priority;
    msg.pid = process_pid;
    msg.shm_id = shm_id;
    msg.mtype = 1; // Any positive number

    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("Forked and sending process %d to scheduler at time %d\n",
           msg.process_id, current_time);

    if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        perror("Error sending process message");
    }

    shmdt(shm_ptr);
    return 1;
}

void sending_arrival_processes(int *next_process_idx, int processCount, int current_time, int *processes_sent)
{
    while (*next_process_idx < processCount &&
           process_list[*next_process_idx].arrival_time <= current_time)
    {
        memory_block_t* memory = allocateMemory(memory_root, process_list[*next_process_idx].memory_size);
        if(memory == NULL)
        {
            printf("added to waiting list\n");
            waiting_list_add(&process_list[*next_process_idx]);
            printf("head %d\n", waiting_list_HEAD->id);
            printf("tail %d\n", waiting_list_TAIL->id);
        }
        else
        {
            
            log_memory_stats(&process_list[*next_process_idx], "allocated", current_time, memory->start, memory->end);
            process_data* process = &process_list[*next_process_idx];
            *processes_sent += sending_process(process, current_time);
            memory->processId = process->pid;
        }
        (*next_process_idx)++;
    }
}

void there_is_no_processes(int processes_sent)
{
    ProcessMessage msg;
    msg.mtype = 1; // Any positive number
    if (processes_sent == 0)
    {
        msg.mtype = 1; // Any positive number
        msg.process_id = -1;
        msg.arrival_time = 0;
        msg.runtime = 0;
        msg.priority = 0;

        if (msgsnd(arrG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
        {
            perror("Error sending no-process message");
        }
    }
}

// Notify the scheduler when a process finishes

void notifySchedulerFinishedProcess(pid_t pid)
{
    CompletionMessage msg;
    msg.mtype = 1;
    msg.process_id = pid;
    msg.finish_time = get_clk();
    memory_block_t* memory = findMemoryBlockByProcessId(memory_root, pid);
    process_data* process = get_process_by_pid(pid);
    log_memory_stats(process, "freed", msg.finish_time, memory->start, memory->end);
    deallocate_memory(memory_root, pid); // Deallocate memory for the finished process   (

    if (msgsnd(compG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        perror("Error sending completion message");
    }
}

process_data* get_process_by_pid(pid_t pid)
{
    for(int i = 0; i < MAX_PROCESSES; i++)
    {
        if (process_list[i].pid == pid)
        {
            return &process_list[i];
        }
    }
    return NULL; // Process not found
}

int waiting_list_remove(process_data* process) {
    if (waiting_list_HEAD == NULL || process == NULL) {
        printf("Error: List is empty or invalid process pointer.\n");
        return 0; // List is empty or invalid process
    }

    if (waiting_list_HEAD == process) {
        printf("Removing process %d from the head of the list.\n", process->id);
        waiting_list_HEAD = waiting_list_HEAD->next;
        if (waiting_list_HEAD == NULL) {
            waiting_list_TAIL = NULL; // List is now empty
        }
        return 1; // Successfully removed
    }

    process_data* current = waiting_list_HEAD;
    while (current->next != NULL && current->next != process) {
        current = current->next;
    }

    if (current->next == process) {
        printf("Removing process %d from the list.\n", process->id);
        current->next = process->next;
        if (waiting_list_TAIL == process) {
            waiting_list_TAIL = current; // Update tail if necessary
        }
        return 1; // Successfully removed
    }

    printf("Process %d not found in the waiting list.\n", process->id);
    return 0; // Process not found
}

void waiting_list_add(process_data* process)
{
    if (waiting_list_HEAD == NULL)
    {
        waiting_list_HEAD = process;
        waiting_list_TAIL = process;
        process->next = NULL;
    }
    else
    {
        waiting_list_TAIL->next = process;
        waiting_list_TAIL = process;
        process->next = NULL;
    }
}

void log_memory_stats(process_data* process, char* state, int current_time, int start, int end) {
    (strcmp(state, "allocated") == 0) ? fprintf(memoryLogFile, "At time %d %s %d bytes for process %d from %d to %d\n",
        current_time, state, process->memory_size, process->id, start, end - 1) :
    fprintf(memoryLogFile, "At time %d %s %d bytes from process %d from %d to %d\n",
        current_time, state, process->memory_size, process->id, start, end - 1); 
    fflush(memoryLogFile);
}

// Function to clean up resources
void clear_resources(int signum)
{
    static int already_cleaning = 0;
    if (already_cleaning)
        return;
    already_cleaning = 1;

    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("Cleaning up resources...\n");


    while(waiting_list_HEAD != NULL)
    {
        waiting_list_remove(waiting_list_HEAD);
    }
    // Remove message queue if it exists
    if (arrG_msgq_id != -1)
    {
        msgctl(arrG_msgq_id, IPC_RMID, NULL);
    }
    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("Arrived Message queue removed\n");

    if (compG_msgq_id != -1)
    {
        msgctl(compG_msgq_id, IPC_RMID, NULL);
    }
    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("Completed Message queue removed\n");

    // Kill scheduler if it exists
    if (scheduler_pid > 0)
    {
        kill(scheduler_pid, SIGINT);
    }
    printf("\033[1;31m");
    printf("[Process Generator] ");
    printf("\033[0m");
    printf("after removeing scheduler\n");

    // Clean up clock resources
    destroy_clk(1);

    exit(0);
}



//os-sim
//arrived remove
//process dec rmeining