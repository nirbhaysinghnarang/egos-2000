#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()
#include <assert.h>
#include "queue.h"
#include "thread.h"
#include <stdint.h>
void terminal_write(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000UL) = str[i];
    }
}

void terminal_write_r(const char *str, int len) {
    // ANSI escape code for red text
    const char red[] = "\033[31m";
    for(int i = 0; i < sizeof(red)-1; i++) {
        *(char*)(0x10000000UL) = red[i];
    }

    // Write the actual string
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000UL) = str[i];
    }

    // Reset color back to default
    const char reset[] = "\033[0m";
    for(int i = 0; i < sizeof(reset)-1; i++) {
        *(char*)(0x10000000UL) = reset[i];
    }
}
extern void ctx_switch(void *old_sp, void* new_sp);
extern void ctx_start(void *old_sp, void* new_sp);

void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            switch (*fmt)
            {
                case 's':
                    strcat(out, va_arg(args, char*));
                    break;
                case 'd':
                    itoa(va_arg(args, int), out + strlen(out), 10);
                    break;
                case 'c':
                    char to_append = (char)va_arg(args, int);  
                    strncat(out, &to_append, 1); 
                    break;
                case 'x':
                    itoa(va_arg(args, int), out + strlen(out), 16);
                    break;
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    char buf[16];  // Enough for 32-bit unsigned (10 digits max)
                    char *p = buf + sizeof(buf) - 1;
                    *p = '\0';
                    do {
                        *--p = '0' + (num % 10);
                        num /= 10;
                    } while (num);
                    strcat(out, p);
                    break;

                }
                case 'p':
                    void* ptr = va_arg(args, void*);  
                    itoa((uintptr_t)ptr, out + strlen(out), 16);  
                    break;
                case 'l':
                    if (*(fmt+1) == 'u') {
                        unsigned long long num = va_arg(args, unsigned long long);
                        char buf[32]; 
                        char *p = buf + sizeof(buf) - 1;
                        *p = '\0';
                        do {
                            *--p = '0' + (num % 10);
                            num /= 10;
                        } while (num);
                        strcat(out, p);
                        fmt++;  // Skip the 'u'
                    }
                default:
                    break;
            }
        }
    }
}



int printf(const char* format, ...) {
    char buf[512] = "";
    va_list args;
    va_start(args, format);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));
    return 0;

}



void log_(const char* message, ...) {
    char buf[512] = "";
  
    // Format the actual message
    char msg_buf[256] = "";
    va_list args;
    va_start(args, message);
    format_to_str(msg_buf, message, args);
    va_end(args);
    
    strcat(buf, msg_buf);
    strcat(buf, "\n\r");
    
    terminal_write_r(buf, strlen(buf));
}


#define MAX_QUEUE_SZ 32
#define STACK_SZ (16 * 1024)  // 16 KB in bytes
static int next_thread_id = 1;  // Start at 1 since main is 0

/**
 * This increases the program's data space by a specified amount, size. 
 */
extern char __heap_start, __heap_max;
static char* brk = &__heap_start;
char* _sbrk(int size) {
    if (brk + size > (char*)&__heap_max) {
        terminal_write("_sbrk: heap grows too large\n\r", 29);
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
// queue_t ready_queue;   
struct thread* current_thread;  
queue_t ready_queue;
void print_saved_registers(void* stack_ptr) {
    uint32_t* regs = (uint32_t*)stack_ptr;
    
    log_("Saved Register Values:\n\r");
    log_("ra  (x1) : %p\n\r", regs[0]);
    log_("t0  (x5) : %p\n\r", regs[1]);
    log_("t1  (x6) : %p\n\r", regs[2]);
    log_("t2  (x7) : %p\n\r", regs[3]);
    log_("t3  (x28): %p\n\r", regs[4]);
    log_("t4  (x29): %p\n\r", regs[5]);
    log_("t5  (x30): %p\n\r", regs[6]);
    log_("t6  (x31): %p\n\r", regs[7]);
    log_("a0  (x10): %p\n\r", regs[8]);
    log_("a1  (x11): %p\n\r", regs[9]);
    log_("a2  (x12): %p\n\r", regs[10]);
    log_("a3  (x13): %p\n\r", regs[11]);
    log_("a4  (x14): %p\n\r", regs[12]);
    log_("a5  (x15): %p\n\r", regs[13]);
    log_("a6  (x16): %p\n\r", regs[14]);
    log_("a7  (x17): %p\n\r", regs[15]);
    log_("s0  (x8) : %p\n\r", regs[16]);
    log_("s1  (x9) : %p\n\r", regs[17]);
    log_("s2  (x18): %p\n\r", regs[18]);
    log_("s3  (x19): %p\n\r", regs[19]);
    log_("s4  (x20): %p\n\r", regs[20]);
    log_("s5  (x21): %p\n\r", regs[21]);
    log_("s6  (x22): %p\n\r", regs[22]);
    log_("s7  (x23): %p\n\r", regs[23]);
    log_("s8  (x24): %p\n\r", regs[24]);
    log_("s9  (x25): %p\n\r", regs[25]);
    log_("s10 (x26): %p\n\r", regs[26]);
    log_("s11 (x27): %p\n\r", regs[27]);
    log_("gp  (x3) : %p\n\r", regs[28]);
    log_("tp  (x4) : %p\n\r", regs[29]);
    log_("sp  (x2) : %p\n\r", regs[30]);
}


void thread_init() {
    //Initialize ready queue
    ready_queue = queue_new();
    if (!ready_queue) {
        log_("Failed to create ready queue\n\r");
        return;
    }    

    current_thread = malloc(sizeof(struct thread));
    if (!current_thread) {
        return;
    }

    //malloc returns a memory address, store that in current_sp
    void* addr = malloc(STACK_SZ);
    current_thread->id = 0; 
    current_thread->current_sp = addr + STACK_SZ;     // Point to the allocated memory address
    current_thread->original_sp = addr ;
    current_thread->entry_fn = NULL;   
    current_thread->arg = NULL;
    current_thread->status = RUNNING;   
}



void thread_create(void (*entry)(void *arg), void *arg){

    if(!current_thread){
        log_("[thread_create] Woah, cowboy! There is no parent environment to inherit from!\n\r");
        return;
    }
    if(queue_length(ready_queue) >= MAX_QUEUE_SZ){
        log_("[thread_create] Could not create thread.\n\r");
        return;
    }    
    //Otherwise, create a thread object
    struct thread* new_thread = malloc(sizeof(struct thread));
    if(!new_thread){
        log_("[thread_create] Not enough memory for thread. [malloc] failed\n\r");
        return;
    }



    int num_threads = queue_length(ready_queue);
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
    log_("[thread_create] Thread %d is READY. We're starting context now.\n\r", new_thread->id);

    ctx_start(
        &(current_thread->current_sp),
        (new_thread->current_sp)
    );
}

// Only supposed to yield to another thread.
void thread_yield() {

 


    log_("[thread_yield] Attempting to yield...\n\r");

    void* thread_ptr;
    struct thread* tmp = current_thread;
    log_("[thread_yield] Current thread ID: %d\n\r", tmp->id);



    if(queue_dequeue(ready_queue, &thread_ptr) != 0){
        log_("[thread_yield] Could not dequeue!");
        return;
    }
    struct thread* new_thread = (struct thread*)thread_ptr;
    log_("[thread_yield] Switching to thread ID: %d\n\r", new_thread->id);
    
    queue_enqueue(ready_queue, current_thread);
    current_thread = new_thread;

    log_("[thread_yield] Context switch from thread ID: %d to thread ID: %d\n\r", tmp->id, new_thread->id);
    log_("[thread_yield] Old SP: %p, NEW SP: %p\n\r", tmp->current_sp, new_thread->current_sp);
    ctx_switch(
        &(tmp->current_sp),
        new_thread->current_sp
    );

    log_("[thread_yield] Finished switch");
}

void thread_exit() {
    log_("[thread_exit] Exiting thread ID: %d\n\r", current_thread->id);
    if (current_thread->original_sp) {
        log_("[thread_exit] Freeing original stack pointer for thread ID: %d\n\r", current_thread->id);
        free(current_thread->original_sp);
        log_("[thread_exit] Finished freeing stack memory\n\r");
    }
    
    void* thread_ptr;
    if (queue_dequeue(ready_queue, &thread_ptr) != 0) {
        log_("[thread_exit] No more threads to run!\n\r");
        while(1) {;} // No more threads, halt
    }

    struct thread* next_thread = (struct thread*)thread_ptr;    
    log_("[thread_exit] Freeing current thread structure for thread ID: %d\n\r", current_thread->id);
    struct thread* to_free = current_thread;    
    current_thread = next_thread;
    free(to_free);
    void* dummy_sp;
    log_("[thread_exit] Switching to thread ID: %d\n\r", next_thread->id);
    ctx_switch(&dummy_sp, next_thread->current_sp);
}



void ctx_entry() {
    log_("[ctx_entry] Called!\n\r");
    if(queue_length(ready_queue) == 0) {
        log_("[ctx_entry] No READY threads found. Entering loop!");
        while (1) {;}
    }
    void* thread_ptr;
    if(queue_dequeue(ready_queue, &thread_ptr) == 0) {

        if (current_thread != NULL) {
            queue_enqueue(ready_queue, current_thread);
        }
        current_thread = (struct thread*)thread_ptr;
        log_("[ctx_entry] Dequeued thread with ID %d\n\r", current_thread->id);
        current_thread->entry_fn(current_thread->arg);
        thread_exit();
    } else {
        log_("[ctx_entry] Dequeue failed!\n\r");
        return;
    }
}


void child(void* arg) {
    char* name = (char*)arg;
    for (int i = 0; i < 5; i++) {
        printf("Thread %s counting: %d\n\r", name, i);
        thread_yield();
    }
}

int main() {
    thread_init();
    
    // Create multiple children with different names
    thread_create(child, "Alpha");
    thread_create(child, "Beta");
    thread_create(child, "Gamma");
    
    // Main thread also counts
    for (int i = 0; i < 5; i++) {
        printf("Main thread counting: %d\n\r", i);
        thread_yield();
    }
    
    thread_exit();
}
