#include "queue.h"

typedef struct {
  int pid;
  int priority;
} process;

Node* createNode(void* data) {
  Node* newnode = (Node*)malloc(sizeof(Node));
  newnode->data = data;
  newnode->next = NULL;
  return newnode;
}

Queue* createQueue() {
  Queue* queue = (Queue*)malloc(sizeof(Queue));
  queue->front = NULL;
  queue->rear = NULL;
  queue->size = 0;
  return queue;
}

int isEmpty(void** queue) {
  Queue* q = (Queue*)queue;
  if(q == NULL)
    return 1;
  return q->size == 0;
}

int getSize(Queue* queue) {
  return queue->size;
}

void printQueue(Queue* queue, void (*printFunc)(void *)) {
  Node* curr = queue->front;
  while (curr != NULL) {
      printFunc(curr->data);
      curr = curr->next;
  }
  printf("\n");
}

void printProcess(void* data) {
  process* p = (process*)data;
  printf("Process ID: %d, Priority: %d /", p->pid, p->priority);
}


void enqueue(Queue* queue, void* data) {
  Node* newnode = createNode(data);
  if (newnode == NULL) {
    printf("Memory allocation failed\n");
    return;
  }
  if(isEmpty(queue)) {
    queue->front = queue->rear = newnode;
  }
  else {
    queue->rear->next = newnode;
    queue->rear = newnode;
  }
  queue->size++;
}

void* dequeue(Queue* queue) {
  if(isEmpty(queue)) {
    printf("Queue is empty\n");
    return NULL;
  }
  
  Node* temp = queue->front;
  void* data = temp->data;
  queue->front = queue->front->next;
  if(queue->front == NULL) {
    queue->rear = NULL;
  }
  free(temp);
  queue->size--;
  return data;
}
