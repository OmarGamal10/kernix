#include <stdio.h>
#include <stdlib.h>
#include "minHeap.h"

// Function to create a new MinHeap with a given capacity and comparison function
MinHeap *createMinHeap(int capacity, int (*compare)(void *a, void *b)) {
    MinHeap *newHeap = malloc(sizeof(*newHeap));
    if (newHeap == NULL) {
        printf("Error in allocating memory for min heap structure\n");
        return NULL;
    }



MinHeap *createMinHeap(int capacity,int (*compare)(void *a,void *b))
{
    MinHeap*newHeap = malloc(sizeof(*newHeap));
    newHeap->capacity = capacity;
    newHeap->size = 0;
    newHeap->array =( void**)malloc(sizeof(void*)*capacity);
    if(newHeap->array == NULL)
    {
        printf("Error in allocating memory for min heap\n");
        free(newHeap);
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

// Function to extract the minimum element from the heap
void *extractMin(MinHeap *min_h) {
    if (min_h->size == 0) {
        printf("Heap is empty\n");
        return NULL;
    }

    // If there is only one element, return it directly
    if (min_h->size == 1) {
        min_h->size--;
        return min_h->array[0];
    }

    // Replace root with last element and reduce size
    void *min = min_h->array[0];
    min_h->array[0] = min_h->array[min_h->size - 1];
    min_h->size--;

    // Call minHeapify to restore heap property
    minHeapify(min_h, 0);
    return min;
}

// Function to get the minimum element without removing it
void *getMin(MinHeap *min_h) {
    if (min_h->size == 0) {
        printf("Heap is empty\n");
        return NULL;
    }
    return min_h->array[0]; // Return the root element (minimum)
}

// Helper function to maintain the MinHeap property
void minHeapify(MinHeap *min_h, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    // Find the smallest among the current node and its children
    if (left < min_h->size && min_h->compare(min_h->array[left], min_h->array[smallest]) < 0)
        smallest = left;

    if (right < min_h->size && min_h->compare(min_h->array[right], min_h->array[smallest]) < 0)
        smallest = right;

    // Swap and recursively fix the affected subtree
    if (smallest != idx) {
        void *temp = min_h->array[idx];
        min_h->array[idx] = min_h->array[smallest];
        min_h->array[smallest] = temp;
        minHeapify(min_h, smallest);
    }
}

// Function to print the heap elements (for testing/debugging)
void printHeap(MinHeap *min_h) {
    for (int i = 0; i < min_h->size; i++) {
        printf("%d ", *(int *)min_h->array[i]);
    }
    printf("\n");
}

// Function to destroy the heap and free its allocated memory
void destroyHeap(MinHeap *min_h) {
    if (min_h == NULL) {
        return;
    }

    free(min_h->array);
    free(min_h);
}
