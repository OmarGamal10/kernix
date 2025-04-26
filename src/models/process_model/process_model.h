#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

enum process_state {
    READY = 0,
    RUNNING = 1,
    WAITING = 2,
    FINISHED = 3,
};

typedef enum process_state process_state;

typedef struct PCB {
    int id;
    int arrival_time;
    int running_time;
    int priority;
    int remaining_time;
    int waiting_time;
    int start_time;
    int finish_time;
    int last_stop_time;
    int turnaround_time;
    process_state status;
    pid_t pid;
} PCB;



PCB *createPCB(int pid, int priority, int arrival_time, int running_time);
void destroyPCB(PCB *pcb);