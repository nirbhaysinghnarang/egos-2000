#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()
#include <assert.h>
#include <stdint.h>

#include "out.h"
#include "queue.h"
#include "thread.h"
#include "condition.h"


//QUESTIONS:
//is null sp/orig sp good enough proxy for main?

//MARK: Define assembly functions
extern void ctx_switch(void *old_sp, void* new_sp);
extern void ctx_start(void *old_sp, void* new_sp);

extern char __heap_start, __heap_max;
static char* brk = &__heap_start;
char* _sbrk(int size) {
    if (brk + size > (char*)&__heap_max) {
        log_e("_sbrk: heap grows too large\n\r", 29);
        return NULL;
    }

    char* old_brk = brk;
    brk += size;
    return old_brk;
}



#define MAX_QUEUE_SZ 32
#define STACK_SZ (16 * 1024)  // 16 KB in bytes
static int next_thread_id = 1;  // Start at 1 since main is 0


// queue_t ready_queue;   
struct thread* current_thread = NULL;  
struct thread* previously_ran_thread = NULL;
queue_t ready_queue = NULL;

void thread_init() {
    //Initialize ready queue
    ready_queue = queue_new();
    if (!ready_queue) {
        log_e("Failed to create ready queue\n\r");
        return;
    }    

    current_thread = malloc(sizeof(struct thread));
    if (!current_thread) {
        return;
    }

    //malloc returns a memory address, store that in current_sp
    current_thread->id = 0; 
    current_thread->current_sp = NULL;    // Point to top of allocated memory.
    current_thread->original_sp = NULL; 
    current_thread->entry_fn = NULL;   
    current_thread->arg = NULL;
    current_thread->status = RUNNING;   
}

void thread_create(void (*entry)(void *arg), void *arg){

    if(!current_thread){
        log_e("[thread_create] Woah, cowboy! There is no parent environment to inherit from!\n\r");
        return;
    }
    if(queue_length(ready_queue) >= MAX_QUEUE_SZ){
        log_e("[thread_create] Could not create thread.\n\r");
        return;
    }    
    //Otherwise, create a thread object
    struct thread* new_thread = malloc(sizeof(struct thread));
    if(!new_thread){
        log_e("[thread_create] Not enough memory for thread. [malloc] failed\n\r");
        return;
    }



    //int num_threads = queue_length(ready_queue);

    new_thread->id = next_thread_id++;
    new_thread->original_sp = malloc(STACK_SZ) ;
    new_thread->current_sp = new_thread->original_sp + STACK_SZ;
    new_thread->entry_fn = entry;
    new_thread->arg = arg;
    new_thread->status = READY;
    //WE WILL BE RUNNING THIS THREAD NEXT - so insert here
    queue_insert(
        ready_queue,
        new_thread
    );
    log_d("[thread_create] Thread %d is READY. We're starting context now.\n\r", new_thread->id);

    ctx_start(
        &(current_thread->current_sp),
        (new_thread->current_sp)
    );
}

void cleanup_terminated_thread() {
    if (!previously_ran_thread) {
        log_d("[cleanup] No thread to cleanup!");
        return;
    }
    
    log_d("[cleanup] Prev Ran Thread ID: %d", previously_ran_thread->id);

    if (previously_ran_thread->original_sp == NULL && 
        previously_ran_thread->status != TERMINATED) {
        log_d("[cleanup] Skipping main thread cleanup - not terminated");
        return;
    }
    
    if (previously_ran_thread->status == TERMINATED) {
        log_d("[cleanup] Freeing terminated thread ID: %d\n\r", previously_ran_thread->id);
        if (previously_ran_thread->original_sp) {
            free(previously_ran_thread->original_sp);
        }
        
        free(previously_ran_thread);
        previously_ran_thread = NULL;
    } else {
        log_d("[cleanup] Thread not terminated, skipping cleanup");
    }
}


void thread_cleanup(){

    //FREE any rogue threads (as long as they're not MAIN)
    if(previously_ran_thread->original_sp){
        free(previously_ran_thread->original_sp);
        free(previously_ran_thread);
    }
    if(current_thread->original_sp){
        free(current_thread->original_sp);
        free(current_thread);
    }

    if(queue_length(ready_queue) != 0){
        log_e("Ooops! The run queue is not empty. This should NEVER happen");
        return;
    }
    free(ready_queue);
    current_thread = NULL;
    previously_ran_thread = NULL;
}

// Only supposed to yield to another thread.
void thread_yield() {
    log_d("[thread_yield] Recvd yield..\n\r");

    cleanup_terminated_thread();
    log_d("[thread_yield] Attempting to yield...\n\r");

    void* thread_ptr;
    struct thread* tmp = current_thread;
    log_d("[thread_yield] Current thread ID: %d\n\r", tmp->id);

    

    if(queue_dequeue(ready_queue, &thread_ptr) != 0){
        log_e("[thread_yield] Could not dequeue!");
        log_e("[thread_yield] ready q has size %d \n\r", queue_length(ready_queue));

        if(current_thread->status == ASLEEP){
            log_d("[thread_yield] Asleep thread, no one to wake\n\r");
            while (1){;}
        }

        
    }
    struct thread* new_thread = (struct thread*)thread_ptr;
    log_d("[thread_yield] Switching to thread ID: %d\n\r", new_thread->id);
    
    
    if (current_thread->status != ASLEEP) { // Only enqueue if not asleep
        queue_enqueue(ready_queue, tmp);
        current_thread->status = READY;
    }    
    
    current_thread = new_thread;

    log_d("[thread_yield] Context switch from thread ID: %d to thread ID: %d\n\r", tmp->id, new_thread->id);
    log_d("[thread_yield] Old SP: %p, NEW SP: %p\n\r", tmp->current_sp, new_thread->current_sp);
    ctx_switch(
        &(tmp->current_sp),
        new_thread->current_sp
    );

    log_d("[thread_yield] Finished switch");
}

void thread_exit() {
    cleanup_terminated_thread();
    log_d("[thread_exit] Exiting thread ID: %d\n\r", current_thread->id);
    current_thread->status = TERMINATED;
    previously_ran_thread = current_thread;
    log_d("[thread_exit] Marked Thread %d TERMINATED.\n\r", previously_ran_thread->id);
    void* thread_ptr;
    if (queue_dequeue(ready_queue, &thread_ptr) != 0) {
        log_d("[thread_exit] No more threads to run!\n\r");
        while(1) {;} // No more threads, halt
        // return;
    }else{
        log_d("[thread_exit] There are %d more threads to run!", queue_length(ready_queue));
    }
    struct thread* next_thread = (struct thread*)thread_ptr;    
    current_thread = next_thread;
    log_d("[thread_exit] Switching to thread ID: %d from thread ID: %d\n\r", next_thread->id, previously_ran_thread->id);
    ctx_switch(&(previously_ran_thread->current_sp), next_thread->current_sp);
}



void ctx_entry() {
    log_d("[ctx_entry] Called!\n\r");
    if(queue_length(ready_queue) == 0) {
        log_e("[ctx_entry] No READY threads found. Entering loop!");
        while (1) {;}
    }
    void* thread_ptr;
    if(queue_dequeue(ready_queue, &thread_ptr) == 0) {
        if (current_thread != NULL) {
            queue_enqueue(ready_queue, current_thread);
        }
        current_thread = (struct thread*)thread_ptr;
        log_d("[ctx_entry] Dequeued thread with ID %d\n\r", current_thread->id);
        current_thread->entry_fn(current_thread->arg);

        thread_exit();
    } else {
        log_e("[ctx_entry] Dequeue failed!\n\r");
        return;
    }
}


/**
 * ASM CODE POINTS HERE.
 */

// int main() {
//     // Stack variables to check if they're preserved across yields
//     run();
//     printf("HEY!\n\r");
//     return 0;
// }
void* buffer[3];
int count = 0;
int head = 0, tail = 0;
struct cv nonempty, nonfull;

void produce(void* item) {
    for (int i = 0; i < 10; i++) {
    // while(1){
        printf("[produce] Pre-wait Current Count is %d\n\r", count);

        while (count == 3) cv_wait(&nonfull);
        printf("[produce] Current Count is %d\n\r", count);

        // At this point, the buffer is not full.
        buffer[tail] = item;
        tail = (tail + 1) % 3;
        count += 1;
        cv_signal(&nonempty);
    }
}

void* consume() {
    while (1) {
        printf("[consume] Pre-wait Current Count is %d\n\r", count);
        while (count == 0) cv_wait(&nonempty);
        printf("[consume] Current Count is %d\n\r", count);

        // At this point, the buffer is not empty.
        void* result = buffer[head];
        head = (head + 1) % 3;
        count -= 1;
        cv_signal(&nonfull);
    }
}



int main() {
    // Initialize the thread library.
    thread_init();
    cv_create(&nonempty, "CONSUMER");
    cv_create(&nonfull, "PRODUCER");
    thread_create((void (*)(void*)) consume, NULL);
    // thread_create((void (*)(void*)) greedy_consume, NULL);

    char *item = "Produced item";
    thread_create(produce, item);
    printf("All done...\n\r");
    thread_exit();

    // Stack variables to check if they're preserved across yields
    return 0;
}