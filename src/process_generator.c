#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>    // for fork, execl
#include <sys/types.h> // for pid_t
#include "clk.h"

void clear_resources(int);
int main(int argc, char * argv[])
{
    pid_t clk_pid = fork();
    if (clk_pid == 0)
    {
        signal(SIGINT, clear_resources);
        sync_clk();
        while (1){
            int x = get_clk();
            printf("current time is %d\n", x);
            sleep(1);
            if (x > 4)
            {
                break;
            }
        }
        // TODO:
        // - A process should spawn at its arrival time
        // - Spawn the scheduler for handling context switching
        destroy_clk(1);
    }
    else
    {
        init_clk();
        sync_clk();
        run_clk();
    }
    
}

void clear_resources(int signum)
{
    //TODO Clears all resources in case of interruption
}
