#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "clk.h"
#include "scheduler.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <errno.h>


#define MAX_PROCESSES 100
#define MAX_LINE_LENGTH 256

// Structure to hold process data
typedef struct
{
    int id;
    int arrival_time;
    int runtime;
    int priority;
    pid_t pid;
    int completed;
    int memsize;
} process_data;

void sigchld_handler(int sig);
int read_processes(const char *filename, process_data process_list[]);
void display_processes(process_data process_list[], int count);
int check_no_more_processes(int processes_sent, int processCount);
void notifySchedulerFinishedProcess(pid_t pid);
void clear_resources(int signum);
void signals_handling();
void arguments_Reader(int argc, char *argv[], int *algorithm_type, int *quantum, int *processCount, process_data process_list[]);
void sending_proceess(int *next_process_idx, int processCount, int current_time, int *processes_sent);
void there_is_no_processes(int processes_sent);
void clear_resources(int);


