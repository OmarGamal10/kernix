#include "process_generator.h"


int processCount = 0;

int arrG_msgq_id = -1; // Message queue ID for arrival messages
int compG_msgq_id = -1; // Message queue ID for completion messages

pid_t scheduler_pid = -1;

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

    signals_handling();
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
                sending_proceess(&next_process_idx, processCount, current_time, &processes_sent);


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
    else if (strcmp(algorithm, "srt") == 0 && argc == 5)
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
        if (sscanf(line, "%d\t%d\t%d\t%d",
                   &process_list[process_count].id,
                   &process_list[process_count].arrival_time,
                   &process_list[process_count].runtime,
                   &process_list[process_count].priority) == 4)
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
    printf("ID\tArrival\tRuntime\tPriority\tPID\n");
    printf("------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        printf("%d\t%d\t%d\t%d\t%d\n",
               process_list[i].id,
               process_list[i].arrival_time,
               process_list[i].runtime,
               process_list[i].priority,
               process_list[i].pid);
    }
}


int check_no_more_processes(int next_process_idx, int processCount)
{
    ProcessMessage msg;
    msg.mtype = 1; // Any positive number

    if (next_process_idx >= processCount)
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

void sending_proceess(int *next_process_idx, int processCount, int current_time, int *processes_sent)
{
    ProcessMessage msg;
    while (*next_process_idx < processCount &&
           process_list[*next_process_idx].arrival_time <= current_time)
    {

        // Create shared memory for this process
        int shm_id = shmget(IPC_PRIVATE, sizeof(int), 0666 | IPC_CREAT);
        if (shm_id == -1)
        {
            perror("Failed to create shared memory");
            continue;
        }

        int *shm_ptr = (int *)shmat(shm_id, NULL, 0);
        if (shm_ptr == (void *)-1)
        {
            perror("Failed to attach shared memory");
            shmctl(shm_id, IPC_RMID, NULL); // Clean up shared memory
            continue;
        }

        *shm_ptr = process_list[*next_process_idx].runtime; // Initialize shared memory with runtime

        // Fork the process
        pid_t process_pid = fork();

        if (process_pid == -1)
        {
            perror("Failed to fork process");
            shmdt(shm_ptr);
            shmctl(shm_id, IPC_RMID, NULL);
            continue;
        }

        if (process_pid == 0)
        { // Child process
            char runtime_str[20], id_str[20], shm_id_str[20];
            sprintf(runtime_str, "%d", process_list[*next_process_idx].runtime);
            sprintf(id_str, "%d", process_list[*next_process_idx].id);
            sprintf(shm_id_str, "%d", shm_id);

            execl("./bin/process", "process", runtime_str, id_str, shm_id_str, NULL);

            perror("Failed to execute process");
            exit(1);
        }

        kill(process_pid, SIGSTOP);
        process_list[*next_process_idx].pid = process_pid;

        msg.process_id = process_list[*next_process_idx].id;
        msg.arrival_time = process_list[*next_process_idx].arrival_time;
        msg.runtime = process_list[*next_process_idx].runtime;
        msg.priority = process_list[*next_process_idx].priority;
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

        (*next_process_idx)++;
        (*processes_sent)++;
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

    if (msgsnd(compG_msgq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        perror("Error sending completion message");
    }
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