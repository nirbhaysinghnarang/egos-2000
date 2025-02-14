
ctx_start:
    addi sp,sp,-128
    SAVE_ALL_REGISTERS
    sw sp,0(a0)       /* Remember the sp of the current thread */
    mv sp,a1          /* Switch to the stack of the newly created thread */
    call ctx_entry    /* Call ctx_entry(), which further calls the entry function of the newly created thread */


### What does [ctx_start] do?

- At a high level, we will call [ctx_start]
when a thread is running, and we create another thread?

* Assume that [sp] is the SP for a new process
* Allocate [128B] of stack space for this function's call frame? 
* Push registers to the stack space just allocated
* [sw sp,0(a0)] will then take the current [sp] and save it into the memory address 
given by the first argument register [a0] -> THIS IS POINTED TO BY THE TCB SO WE CAN 
RESTORE IT

* Next, switch the SP to the SP of the newly created thread.





ctx_switch:
    addi sp,sp,-128
    SAVE_ALL_REGISTERS
    sw sp,0(a0)       /* Remember the sp of the current thread */
    mv sp,a1          /* Switch to the stack of the next thread */
    RESTORE_ALL_REGISTERS
    addi sp,sp,128
    ret



### What does [ctx_switch] do?

    *The key difference is*:

    * [ctx_start] jumps to the thread's entry point (it's never run before)
    * [ctx_switch] restores and returns to wherever the thread was last running

    Here's the sequence:

    First time switching to a new thread: use [ctx_start]
    All subsequent switches between threads: use [ctx_switch]



* In other words, we now have two stacks in the memory, which is the key feature of multithreaded programs: one stack for each thread.

### TCB

thread id: a unique identifier of a thread
status: the thread’s current execution state
stack pointer: value of register sp before switching to another thread
entry function: the “main” function of a thread (e.g., child function for the child thread)
entry function arguments: argument(s) of the thread’s “main” function
other metadata that could be useful
In P1, you can implement the thread control block as follows.

c
struct thread {
    int id;
    void* sp;
    ...                // Design the rest of struct thread yourself.
};
struct thread TCB[32]; // Assume at most 32 threads for simplicity.
int current_idx;       // TCB[current_idx] is the currently running thread.