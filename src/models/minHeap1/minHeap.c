
#include "stdlib.h"
#include "minHeap.h"



MinHeap *createMinHeap(int capacity,int (*compare)(void *a,void *b))
{
    MinHeap*min_h;
    MinHeap*newHeap = malloc(sizeof(*newHeap));
    newHeap->capacity = capacity;
    newHeap->size = 0;
    newHeap->array =( void**)malloc(sizeof(void*)*capacity);
    if(min_h->array == NULL)
    {
        printf("Error in allocating memory for min heap\n");
        return NULL;
    }

    return min_h;
}