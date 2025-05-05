#include<stdlib.h>
#include "list.h"
#include "memory.h"

void* get_hole(List *l,int size){

    // If list is empty
    if(l->start == NULL){
        return (void*)-1;
    }

    void *ret = (void*)-1;

    Node *prev = l->start;
    Node *temp = l->start;

    // Only one node;
    if(l->start->next == NULL){
        if(l->start->size < size){
            // No sufficient sized chunk available
            return ret;
        }

        l->start = NULL;
        ret = temp->loc;
        free(temp);
        return ret;
    }
    
    //* Following logic should be imporved, by using a different traversing stratergy.

    // More than one node
    while(temp->next != NULL && temp->next->size < size){
        prev = temp;
        temp = temp->next;
    }
    if(temp->next == NULL && temp->size < size){
        // Last node,not adequate size.
        return (void*)-1;
    }
    // last node with adequate size
    if(temp->next == NULL){
        prev->next = NULL;
        ret = temp->loc;
        free(temp);
        return ret;
    }
    // Now we are on Node whose next node is of adequate size.
    // So move on next
    // We found adequate sized hole!!!
    prev = temp;
    temp = temp->next;
    
    prev->next = temp->next;
    ret = temp->loc;
    free(temp);
    return ret;
}

void insert_hole(List *l,int size,void*loc){
    Node *newnode = (Node *)malloc(sizeof(Node));
    newnode->loc = loc;
    newnode->size = size;
    newnode->next = NULL;

    // Empty list
    if(l->start == NULL){
        l->start = newnode;
        return;
    }
    // First Node is of size lrger than the one we are inserting
    if(l->start->size > size ){
        newnode->next = l->start;
        l->start = newnode;
        return;
    }
    // Traverse and find
    Node* temp = l->start;
    while(temp->next != NULL && temp->next->size <= size){
        temp = temp->next;
    }
    // Found it!
    newnode->next = temp->next;
    temp->next =newnode;
    return;
    
}

void* get_adjecent_hole(List *l,void *loc,int size){

    Node *temp = l->start;
    Node *prev = l->start;

    void * ret = (void*)-1;

    while(temp != NULL && temp->size != size){
        prev = temp;
        temp = temp->next;
    }
    // No node with given size is found
    if(temp == NULL) {
        return (void*)-1;
    }

    Chunk *in = (Chunk *)loc;
    // There can be multiple holes with same size.
    // Here we check if any of them is adjecent and return first one found so.
    while(temp != NULL && temp->size == size){
        Chunk *t = (Chunk *)temp->loc;
        if(in - in->size == t || in + in->size == t){
            // Found an adjecent Node Hole.
            if(l->start == temp){
                // If the Fisrt Node itself in list is adjecent
                l->start = temp->next;
            }else{
                prev->next = temp->next;
            }
            ret = temp->loc;
            free(temp);
            return ret;
        }
        prev = temp;
        temp = temp->next;
    }
    // No Hole of required size was adjecent.
    return (void*)-1;

}