
#include "clk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Modify this file as needed*/
int remaining_time = 0; // Placeholder for remaining time
void run_process(int runtime, int id, int *current_shm_ptr)
{
    printf("Process with id %d started at time %d, in process\n", id, get_clk());
    printf("Process with id %d has remaing time %d , in process\n", id, remaining_time);
    //process terminates when the shared memory value reaches 0
    while (remaining_time > 0)
    {
        remaining_time = *current_shm_ptr; // Get the remaining time from shared memory
        printf("Process with id %d is running at time %d, remaining time: %d\n", id, get_clk(), remaining_time);
        sleep(.5);
        //sleep(0.5); // Simulate process work
        // it is decremented in the scheduler
    }
    printf("Process with id %d finished at time %d,, in process\n", id, get_clk());
}

int main(int argc, char *argv[])
{
    sync_clk(); // Establish communication with the clock module
    if (argc != 4)
    {
        printf("Usage: %s <runtime>\n", argv[0]);
        return 1;
    }
    int runtime = atoi(argv[1]);
    int id = atoi(argv[2]);
    int shm_id = atoi(argv[3]);
    
    printf("Process %d attempting to attach to shared memory %d\n", id, shm_id);
    
    int *current_shm_ptr = (int *)shmat(shm_id, NULL, 0);
    if ((long)current_shm_ptr == -1)
    {
        perror("Failed to attach shared memory segment1");
        return 1;
    }

    //*current_shm_ptr = runtime; // Initialize shared memory with runtime value
    
    printf("Process %d successfully attached to shared memory %d\n", id, shm_id);
    remaining_time = *current_shm_ptr; // Get the initial remaining time from shared memory
    printf("Process %d has remaining time inn processsssssss %d\n", id, remaining_time);
    
    if (runtime <= 0)
    {
        printf("Invalid runtime value. It should be a positive integer.\n");
        return 1;
    }
    
    run_process(runtime, id, current_shm_ptr);
    // Detach shared memory
    if (shmdt(current_shm_ptr) == -1)
    {
        perror("Failed to detach shared memory segment");
        return 1;
    }
    destroy_clk(0);
    exit(0); // Exit the process

}


