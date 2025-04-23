# os-sim
**Read this section carefully as it overrides anything in the pdf.**


A simulation for a mini-os implementing scheduling and memory management techniques.

- Feel free to change the code structure as you want, your mini-kernel will be called as follows.
```bash
make
./os-sim -s <scheduling-algorithm> -f <processes-text-file>
./os-sim -s rr -f processes.txt
./os-sim -s hpf -f processes.txt
./os-sim -s srtn -f processes.txt
```

- It's the `process_generator` responsibility to spawn processes, clock, scheduler, memory management unit, ...etc.
- A process is only spawned when it's arrival time ticks, then passed to the scheduler for proper scheduling.
- It's the process responsibility to terminate itself when it's runtime finishes.
- A team shall consist of 3-4 members.
- Refer to `Project Phase 1.pdf` for more info. Expect Phase 2 to be added afterwards while adding some memory management requirements. Make sure your code is **modular** and **extendable**.