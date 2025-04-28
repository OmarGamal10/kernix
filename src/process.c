#include "clk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int remaining_time = 0; // Placeholder for remaining time


// Function to simulate the process execution
void run_process(int runtime, int id, int *current_shm_ptr)
{
    // Process terminates when the shared memory value reaches 0

    while (remaining_time > 0)
    {
        remaining_time = *current_shm_ptr; // Get the remaining time from shared memory
    }
}

int main(int argc, char *argv[])
{
    sync_clk(); // Establish communication with the clock module

    // Validate the number of arguments
    if (argc != 4)
    {
        printf("Usage: %s <runtime>\n", argv[0]);
        return 1;
    }

    // Parse command-line arguments
    int runtime = atoi(argv[1]); // Runtime of the process
    int id = atoi(argv[2]);      // Process ID
    int shm_id = atoi(argv[3]);  // Shared memory ID

    // Attach to the shared memory segment

    int *current_shm_ptr = (int *)shmat(shm_id, NULL, 0);
    if ((long)current_shm_ptr == -1)
    {
        perror("Failed to attach shared memory segment1");
        return 1;
    }


    remaining_time = *current_shm_ptr; // Get the initial remaining time from shared memory

    if (runtime < 0)
    {
        printf("Invalid runtime value. It should be a positive integer.\n");
        return 1;
    }


    // Run the process simulation
    run_process(runtime, id, current_shm_ptr);

    // Detach from the shared memory segment

    if (shmdt(current_shm_ptr) == -1)
    {
        perror("Failed to Deattach shared memory segment");
        return 1;
    }

    destroy_clk(0); // Clean up clock communication
    exit(0);        // Exit the process

}
