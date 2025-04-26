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

int isEmpty(Queue* queue) {
  return queue->size == 0;
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
  //printQueue(queue, printProcess);
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
  //printQueue(queue, printProcess);
  return data;
}


/*
int main() {
  struct Queue* q = createQueue();

  process* a = malloc(sizeof(process));
  a->pid = 1;
  a->priority = 5;
  enqueue(q, a);

  process* b = malloc(sizeof(process));
  b->pid = 20;
  b->priority = 10;
  enqueue(q, b);

  process* f = (process*)dequeue(q);
  printf("Dequeued: %d %d\n", f->pid, f->priority);
  f = (process*)dequeue(q);
  printf("Dequeued: %d %d\n", f->pid, f->priority);

  f = (process*)dequeue(q);
  if(f == NULL) {
    printf("Queue is empty\n");
  } else {
    printf("Dequeued: %d %d\n", f->pid, f->priority);
  }

  process* c = malloc(sizeof(process));
  c->pid = 30;
  c->priority = 15;
  enqueue(q, c);

  process* d = malloc(sizeof(process));
  d->pid = 40;
  d->priority = 20;
  enqueue(q, d);

  process* e = malloc(sizeof(process));
  e->pid = 50;
  e->priority = 25;
  enqueue(q, e);


  f = (process*)dequeue(q);
  printf("Dequeued: %d %d\n", f->pid, f->priority);
  
  return 0;
}
  */