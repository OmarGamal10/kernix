#pragma once
#include "process_model/process_model.h"

//Define algorithm constants
#define HPF 1
#define SRT 2
#define RR 3

//function prototypes
void initialization(int algorithm, int q);

void recieve_process();
void scheduleProcess();

void scheduleHPF();
void scheduleSRT();
void scheduleRR();

void start_process(int process_id);
void stop_process(int process_id);
void destroy_process(int process_id);

void logProcessStateChange(PCB *pcb, process_state new_state);
void updateWaitingTime();
void updateStatistics(PCB *pcb);
void generateStatistics();

void cleanup();

//comparsion functions for HPF and SRT
int comparePriority(void *a, void *b);
int compareRemainingTime(void *a, void *b);
// int compareByID(void *a, void *b);

