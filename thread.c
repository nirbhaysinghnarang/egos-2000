void terminal_write(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000UL) = str[i];
    }
}

#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()
#include "thread.h"
#include "queue.h"

extern void ctx_switch(void *old_sp, void* new_sp);
extern void ctx_start(void *old_sp, void* new_sp);



void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            if (*fmt == 's') {
                strcat(out, va_arg(args, char*));
            } else if (*fmt == 'd') {
                itoa(va_arg(args, int), out + strlen(out), 10);
            }
        }
    }
}

int printf(const char* format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));

    return 0;
}

#define MAX_QUEUE_SZ 32
#define STACK_SZ (16 * 1024)  // 16 KB in bytes

/**
 * This increases the program's data space by a specified amount, size. 
 */
extern char __heap_start, __heap_max;
static char* brk = &__heap_start;
char* _sbrk(int size) {
    if (brk + size > (char*)&__heap_max) {
        terminal_write("_sbrk: heap grows too large\r\n", 29);
        return NULL;
    }

    char* old_brk = brk;
    brk += size;
    return old_brk;
}



struct thread {
    int id;                     // Thread ID
    void* current_sp;           // Stack pointer
    void (*entry_fn)(void*);    // Function pointer for entry function
    void* arg;                  // Pointer to argument(s)
    enum {
        READY,
        RUNNING,
        BLOCKED,
        TERMINATED,
        PRIMORDIAL              //Before there was any light!
    } status;                   // Thread status
    void* original_sp;          // For dealloc
};
queue_t ready_queue;   
struct thread* current_thread;  



void thread_init() {
    ready_queue = queue_new();
    if (!ready_queue) {
        printf("Failed to create ready queue\n");
        return;
    }    


    //Create MASTER thread;?
    struct thread* master_thread = malloc(sizeof(struct thread));
    

}


void thread_create(void (*entry)(void *arg), void *arg){
    if(queue_length(ready_queue) >= MAX_QUEUE_SZ){
        printf("[thread_create] Could not create thread.");
        return;
    }    
    //Otherwise, create a thread object
    struct thread* new_thread = malloc(sizeof(struct thread));
    if(!new_thread){
        printf("[thread_create] Not enough memory for thread. [malloc] failed");
        return;
    }
    int num_threads = queue_length(ready_queue);
    new_thread->id = num_threads+1;
    new_thread->original_sp = malloc(STACK_SZ);
    new_thread->current_sp = new_thread->original_sp;
    new_thread->entry_fn = entry;
    new_thread->arg = arg;
    new_thread->status = READY;
    queue_enqueue(
        ready_queue,
        new_thread
    );
    printf("[thread_create] Thread %d is READY.", num_threads+1);
}


void ctx_entry(){
    if(queue_length(ready_queue) == 0){
        printf("[ctx_entry] No READY threads found. Entering loop!");
        while (1) {;}
    }
   void* thread_ptr;
    if(queue_dequeue(ready_queue, &thread_ptr) == 0) {
        current_thread = (struct thread*)thread_ptr;
        printf("[ctx_entry] Dequeued thread with ID %d\n", current_thread->id);
        current_thread->entry_fn(current_thread->arg);
        //Call [thread_exit]?

    }else{
         printf("[ctx_entry] Dequeue failed!");
         return;
    }
}



void thread_yield(){

}
int main() {
    printf("Start to implement multi-threading!\n\r");

    return 0;
}
