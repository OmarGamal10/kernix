#ifndef LIST
#define LIST

//* Individual Node in a list. Contains size and location of hole, pointer to next Node
typedef struct Node{
    int size;
    void* loc;
    struct Node *next;
} Node;

typedef struct List{
    Node *start;
} List;

//* Removes a holes from list l that can accomodate size, i.e. hole-size >= size
void* get_hole(List *l,int size);

//* Adds hole to the list l of size size and which starts at location loc
//* List is ordered small-size -> large-size
void insert_hole(List *l,int size,void* loc);

//* Finds a hole in list whose location is adjecent to loc and is of size size.
//* Adjecent i.e. hole which ends at loc, or hole which starts at loc+size
void* get_adjecent_hole(List *l,void *loc,int size);

#endif