
#include "clk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Modify this file as needed*/

void run_process(int runtime, int id, int *current_shm_ptr)
{
    //process terminates when the shared memory value reaches 0
    while (*current_shm_ptr > 0)
    {
        //sleep(0.5); // Simulate process work
        // it is decremented in the scheduler
    }
    printf("Process with id %d finished at time %d\n", id, get_clk());
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
    int *current_shm_ptr = (int *)shmat(shm_id, NULL, 0);
    if (current_shm_ptr == (int *)-1)
    {
        perror("Failed to attach shared memory segment");
        return 1;
    }
    if (runtime <= 0)
    {
        printf("Invalid runtime value. It should be a positive integer.\n");
        return 1;
    }
    ;
    run_process(runtime, id, current_shm_ptr);
    // Detach shared memory
    if (shmdt(current_shm_ptr) == -1)
    {
        perror("Failed to detach shared memory segment");
        return 1;
    }
    destroy_clk(0);

    return 0;
}
