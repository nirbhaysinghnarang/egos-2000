#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

//TODO: Add invariants and assertions here
!

/**
 We need enqueue to be O(1)
 so head tail ptrs
 where each node has a next ptr
 **/

#define SUCCESS 0;
#define FAILURE -1;


typedef struct node {
    void* data;
    //need struct here because of recursive struct
    struct node* next;
} node_t; //If we add a pointer here, don't need a [star] below?





typedef struct queue {
    node_t* head;
    node_t* tail;
    int size;
} *queue_t;

queue_t queue_new() {
    //Allocate memory
    queue_t q = malloc(sizeof(struct queue));
    
    //If alloc fails, return null
    if(q == NULL){
        return NULL;
    }
    
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    
    return q;
}

node_t* create_node(void* item){
    node_t* n = malloc(sizeof(struct node));
    
    if (n == NULL){
        return n;
    }
    n->data = item;
    n->next = NULL;
    return n;
    
}


int queue_enqueue(queue_t queue, void* item) {
    assert(queue);

    // your code here
    // Must be O(1) (performance does not depend on length of queue)
    //Allocate memory for new item
    node_t* n = create_node(item);
    if(n == NULL){
        return FAILURE;
    }
    //Is queue empty? If so, assign to head/tail, incr size, return
    if(queue->size == 0){
        queue->head = n;
        queue->tail = n;
        queue->size++;
        return SUCCESS;
    }
    //If not, get tail, set next
    node_t* prev_tail = queue->tail;
    prev_tail->next = n;
    queue->tail = n;
    queue->size++;
    return SUCCESS;

}

int queue_insert(queue_t queue, void* item) {
    assert(queue);

    // your code here
    // Must be O(1) (performance does not depend on length of queue)
    node_t* n = create_node(item);
    if(n == NULL){
        return FAILURE;
    }
    if(queue->size == 0){
        queue->head = n;
        queue->tail = n;
        queue->size++;
        return SUCCESS;
    }
    
    
    node_t* prev_head = queue->head;
    n->next = prev_head;
    queue->head = n;
    queue->size++;
    return SUCCESS;

}

int queue_dequeue(queue_t queue, void** pitem) {
    assert(queue);

    // your code here
    // Must be O(1) (performance does not depend on length of queue)


    //Why do we need a dbl pointer here?
    //[**pitem] is a pointer to the item that was just dequeued.
    //because the item that was just dequeued is itself a pointer, we need
    //this to be a pointer to a pointer.

    if(queue->size == 0){
        return FAILURE;
    }


    if(pitem == NULL){
	    node_t* old_head = queue->head;
        //just dequeue, don't do anything to pitem
        if(queue->size == 1){
            queue->head = NULL;
            queue->tail = NULL;
            queue->size = 0;
            free(old_head);
            return SUCCESS;
            
        }
        node_t* new_head = (queue->head)->next;
        queue->head = new_head;
        queue->size--;
	    free(old_head);
        return SUCCESS;
        
    }else{
	    node_t* old_head = queue->head;
        if(queue->size == 1){
            void* data = (queue->head)->data;
            //Point to data;
            *pitem = data;
            queue->head = NULL;
            queue->tail = NULL;
            queue->size = 0;
            free(old_head);
            return SUCCESS;
        }
        void* data = (queue->head)->data;
        //Point to data;
        *pitem = data;
        node_t* new_head = (queue->head)->next;
        queue->head = new_head;
        queue->size--;
	    free(old_head);
        return SUCCESS;
    } 
}

void queue_iterate(const queue_t queue, queue_func_t f, void* context) {
    assert(queue);
    assert(f);

    // your code here
    if(queue->size == 0){
        return;
    }
    //Get current node.
    node_t* current = queue->head;
    while (current != NULL){



        /* Per function comment "Call f(item, context) for each item in queue",
          we pass the item data rather than the node itself to maintain
          encapsulation of the queue's internal node structure */

          
        f(current->data, context);
        current = current->next;
    }
}



int queue_free(queue_t queue) {
    assert(queue);
    // your code here
    // Must be O(1) (performance does not depend on length of queue)
    if(queue->size != 0){
        return FAILURE;
    }
    free(queue);
    return SUCCESS;
}

int queue_length(const queue_t queue) {
    assert(queue);
    // your code here
    // Must be O(1) (performance does not depend on length of queue)
    return queue->size;
}

int queue_delete(queue_t queue, void* item) {
    assert(queue);
    if (queue->size == 0) {
        return FAILURE;
    }

    node_t* prev = NULL;
    node_t* current = queue->head;

    while (current != NULL && current->data != item) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        return FAILURE;
    }

    if (prev == NULL) {
        queue->head = current->next;
    } else {
        prev->next = current->next;
    }

    if (current->next == NULL) {
        queue->tail = prev;
    }


    //Do we need to free the memory location pointed to by current->data?
    //free(current->data)
    free(current);
    queue->size--;

    return SUCCESS;
}

//And we're done