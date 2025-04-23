
#include "stdlib.h"
#include "minHeap.h"



MinHeap *createMinHeap(int capacity,int (*compare)(void *a,void *b))
{
    MinHeap*newHeap = malloc(sizeof(*newHeap));
    newHeap->capacity = capacity;
    newHeap->size = 0;
    newHeap->array =( void**)malloc(sizeof(void*)*capacity);
    if(newHeap->array == NULL)
    {
        printf("Error in allocating memory for min heap\n");
        return NULL;
    }

    return newHeap;
}


void insertMinHeap (MinHeap * min_h, void *data)
{
    if(min_h->size == min_h->capacity)
    {
        printf("Heap is full\n");
        return;
    }

    min_h->size++;
    min_h->array[min_h->size - 1] = data;

    int i = min_h->size - 1;
    while(i != 0 && min_h->compare(min_h->array[i], min_h->array[(i - 1) / 2]) < 0)
    {
        void* temp = min_h->array[i];
        min_h->array[i] = min_h->array[(i - 1) / 2];
        min_h->array[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }

}


int getElementIndex(MinHeap *min_h, void *data)
{
    for(size_t i = 0; i < min_h->size; i++)
    {
        if(min_h->array[i] == data)
            return i;
    }
    return -1;
}