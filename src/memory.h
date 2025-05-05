/**
 * memory_block_t
 * A struct to represent a memory block in the memory
 */
typedef struct memory_block {
    int size;
    int realSize;
    int start;
    int end;
    int processId;
    int isFree;
  
    // tree like pointers
    struct memory_block *parent;
    struct memory_block *left;
    struct memory_block *right;
  } memory_block_t;
  
  // =============================================================================
  // INTERNAL FUNCTIONS
  int highestPowerOf2(int x);
  memory_block_t *initializeMemoryBlock(int size, int start, int end);
  memory_block_t *findMemoryBlock(memory_block_t *root, int addr);
  memory_block_t *findMemoryBlockByProcessId(memory_block_t *root, int processId);
  
  // API
  memory_block_t *initMemory();
  memory_block_t *allocateMemory(memory_block_t *root, int size, int processId);
  void freeMemory(memory_block_t *root, int processId);
  int isThereEnoughSpaceFor(memory_block_t *root, int size);
  
  // output functions
  void createMemoryLogFile();
  void memoryLogger(memory_block_t *root, int time, const char *message,
                    int processId, int size);
  void fancyPrintTree(memory_block_t *root, int level);
  void fancyPrintMemoryBar(memory_block_t *root);