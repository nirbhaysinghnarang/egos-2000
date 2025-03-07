#include "queue.h"
#include "out.h"
#include "thread.h"
#include "condition.h"
#include <stdlib.h>  // for itoa()

static int cv_id_counter = 0;


// A thread will invoke cv_wait or cv_signal on 
//a conditional variable.

//Sketching out a possible solution below
//[cv] contains two fields: waiting (bool) and list of waiting threads (is the first even needed - no!)
//[cv] simply contains a list of waiting threads
//on [cv_wait], add calling thread to list...change status to waiting in TCB (somehow pause execution? - trap in while loop with yield?)
//on [cv_signal] change all threads to READY (add to ready queue?)


void cv_create(struct cv* condition, char* name){
    condition->wait_queue = queue_new();
    condition->id = cv_id_counter++;
    condition->name = name;
}

void cv_wait(struct cv* condition){
    //Get currently running thread.
    //Add to condition wait queue.
    //change status
    //while status is unchaned,yield

    log_d("Adding thread %d to CV %s's wait queue", current_thread->id, condition->name);
    if(queue_insert(condition->wait_queue,current_thread) != 0) {
        log_d("[cv_wait] Could not enqueue for CV %s", condition->name);
    }else{
        log_d("[cv_wait] Enqueued thread %d for CV %s", current_thread->id, condition->name);
    }
    current_thread->status = ASLEEP;
    while(current_thread->status == ASLEEP){
        log_d("[cv_wait] Asleep, yielding\n\r");
        thread_yield();
    }
}

void cv_signal(struct cv* condition) {
    struct thread* waiting_thread = NULL;
    if (queue_dequeue(condition->wait_queue, (void**)&waiting_thread) == 0) {
        log_d("Removing thread %d from CV %s's wait queue", waiting_thread->id, condition->name);
        waiting_thread->status = READY;
        queue_enqueue(ready_queue, waiting_thread);
        log_d("[cv_signal] Yielding!\n\r");
        log_d("Inserted thread %d to ready queue.", waiting_thread->id);
    }else{
        log_d("Dequeue failed for CV %s", condition->name);
    }
}

void cv_release(struct cv* condition){
    if(queue_length((condition->wait_queue)) == 0){
        queue_free(condition->wait_queue);
        log_d("CV %s is a free elf!", condition->name);
    }else{
        log_e("Oops, big man! Can't release a CV with threads hanging!");
    }
}