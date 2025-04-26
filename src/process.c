#include "clk.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Modify this file as needed*/

void run_process(int runtime, int id)
{
    sync_clk();
    int start_time = get_clk();
    int end_time = start_time + runtime;
    while(get_clk() < end_time)
    {
        int cur = get_clk();
        while(get_clk() == cur)
        {
            // Busy wait
        }
        printf("Process with id %d running at time %d\n", id, get_clk());
    }
    printf("Process with id %d finished at time %d\n", id, get_clk());
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <runtime>\n", argv[0]);
        return 1;
    }

    int runtime = atoi(argv[1]);
    int id = atoi(argv[2]);
    if (runtime <= 0)
    {
        printf("Invalid runtime value. It should be a positive integer.\n");
        return 1;
    }

    run_process(runtime, id);
    destroy_clk(0);

    return 0;
}