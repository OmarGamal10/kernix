#pragma once
#include "stdlib.h"
#include <stdbool.h>
#include "stdio.h"

typedef struct Node {
  void* data;
  struct Node* next;
} Node;


typedef struct Queue {
  Node* front;
  Node* rear;
  int size;
} Queue;

Node* createNode(void* data);
Queue* createQueue();
int isEmpty(void *queue);
int getSize(Queue* queue);
void printQueue(Queue* queue, void (*printFunc)(void *));
void enqueue(Queue* queue, void* data);
void* dequeue(Queue* queue);