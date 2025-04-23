#pragma once
#include "stdlib.h"
#include "stdio.h"



typedef struct MinHeap
{
    void **array;
    int capacity;
    int size;
    int (*compare)(void *a, void *b);
} MinHeap;


MinHeap *createMinHeap(int capacity, int (*compare)(void *a, void *b));