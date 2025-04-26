
#include "process_model.h"
#include <stdio.h>
#include <stdlib.h>

PCB* createPCB(int pid, int priority, int arrival_time, int running_time) {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    if (pcb == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    pcb->pid = pid;
    pcb->priority = priority;
    pcb->arrival_time = arrival_time;
    pcb->running_time = running_time;
    pcb->remaining_time = running_time;
    pcb->waiting_time = 0;
    pcb->turnaround_time = 0;
    pcb->start_time = -1;
    pcb->finish_time = -1; 
    pcb->status = READY;
    return pcb;
}

void destroyPCB(PCB *pcb) {
    if (pcb != NULL) {
        free(pcb);
    }
}