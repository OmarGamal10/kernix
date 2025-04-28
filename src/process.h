#ifndef PROCESS_H
#define PROCESS_H

/**
 * @brief Executes a process for a specified runtime.
 * 
 * This function simulates the execution of a process by running it
 * for a given amount of time. It also interacts with shared memory
 * to update or retrieve process-related data.
 * 
 * @param runtime The duration (in arbitrary time units) for which the process should run.
 * @param id The unique identifier of the process to be executed.
 * @param current_shm_ptr Pointer to the shared memory segment used for inter-process communication.
 */
void run_process(int runtime, int id, int *current_shm_ptr);

#endif