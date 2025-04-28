#include <stdio.h>
#include <stdlib.h>
#include "minHeap.h"

// Create a new MinHeap with a given capacity and comparison function
MinHeap *createMinHeap(int capacity, int (*compare)(void *a, void *b))
{
    MinHeap *newHeap = malloc(sizeof(*newHeap)); // Allocate memory for the heap
    newHeap->capacity = capacity;
    newHeap->size = 0;
    newHeap->array = (void **)malloc(sizeof(void *) * capacity); // Allocate memory for the array
    if (newHeap->array == NULL) // Check for allocation failure
    {
        printf("Error in allocating memory for min heap\n");
        free(newHeap);
        return NULL;
    }
    newHeap->compare = compare; // Set the comparison function
    return newHeap;
}

// Insert a new element into the MinHeap
void insertMinHeap(MinHeap *min_h, void *data)
{
    if (min_h->size == min_h->capacity) // Check if the heap is full
    {
        printf("Heap is full\n");
        return;
    }

    min_h->size++;
    min_h->array[min_h->size - 1] = data; // Add the new element at the end

    int i = min_h->size - 1;
    while (i != 0 && min_h->compare(min_h->array[i], min_h->array[(i - 1) / 2]) < 0)
    {
        // Swap the element with its parent to maintain the heap property
        void *temp = min_h->array[i];
        min_h->array[i] = min_h->array[(i - 1) / 2];
        min_h->array[(i - 1) / 2] = temp;
        i = (i - 1) / 2;
    }
}

// Get the index of a specific element in the heap
int getElementIndex(MinHeap *min_h, void *data)
{
    for (size_t i = 0; i < min_h->size; i++) // Iterate through the heap
    {
        if (min_h->array[i] == data) // Check if the element matches
            return i;
    }
    return -1; // Return -1 if not found
}

// Extract the minimum element from the heap
void *extractMin(MinHeap *min_h)
{
    if (min_h->size == 0) // Check if the heap is empty
    {
        return NULL;
    }

    if (min_h->size == 1) // If only one element, return it
    {
        min_h->size--;
        return min_h->array[0];
    }

    void *min = min_h->array[0]; // Store the minimum element
    min_h->array[0] = min_h->array[min_h->size - 1]; // Replace root with the last element
    min_h->size--;

    minHeapify(min_h, 0); // Restore the heap property
    return min;
}

// Get the minimum element without removing it
void *getMin(MinHeap *min_h)
{
    if (min_h->size == 0) // Check if the heap is empty
    {
        return NULL;
    }
    return min_h->array[0]; // Return the root element
}

// Maintain the MinHeap property for a subtree
void minHeapify(MinHeap *min_h, int idx)
{
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < min_h->size && min_h->compare(min_h->array[left], min_h->array[smallest]) < 0)
        smallest = left; // Find the smallest among the current node and left child

    if (right < min_h->size && min_h->compare(min_h->array[right], min_h->array[smallest]) < 0)
        smallest = right; // Find the smallest among the current node and right child

    if (smallest != idx) // If the smallest is not the current node
    {
        void *temp = min_h->array[idx];
        min_h->array[idx] = min_h->array[smallest];
        min_h->array[smallest] = temp;
        minHeapify(min_h, smallest); // Recursively heapify the affected subtree
    }
}

// Print the heap elements (for debugging)
void printHeap(MinHeap *min_h)
{
    for (int i = 0; i < min_h->size; i++) // Iterate through the heap
    {
        printf("%d ", *(int *)min_h->array[i]); // Assumes elements are integers
    }
    printf("\n");
}

// Destroy the heap and free its memory
void destroyHeap(MinHeap *min_h)
{
    if (min_h == NULL) // Check if the heap is already NULL
        return;

    free(min_h->array); // Free the array
    free(min_h); // Free the heap structure
}

