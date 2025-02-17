#ifndef THREAD_H
#define THREAD_H

#include "queue.h"  // Add this if queue_t is needed

struct thread {
    int id;                    
    void* current_sp;          
    void (*entry_fn)(void*);   
    void* arg;                 
    enum {
        READY,
        RUNNING,
        ASLEEP    
    } status;                  
    void* original_sp;         
};

void thread_init();
void thread_create(void (*entry)(void *arg), void *arg);
void thread_yield();
void thread_exit();

// Only declare the globals with extern
extern struct thread* current_thread;
extern queue_t ready_queue;

#endif