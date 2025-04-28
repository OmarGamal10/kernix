#include <stdio.h>
#include <stdlib.h>

// Structure to hold process data
typedef struct
{
    int id;
    int arrival_time;
    int runtime;
    int priority;
    pid_t pid;
    int completed;
} process_data;

void sigchld_handler(int sig);
int read_processes(const char *filename, process_data process_list[]);
void display_processes(process_data process_list[], int count);
void check_completed_processes(process_data *process_list, int num_processes);
void notifySchedulerFinishedProcess(pid_t pid);
void clear_resources(int signum);


