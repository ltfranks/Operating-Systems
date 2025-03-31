#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define DEFAULT_STACK_SIZE 1024*1024
#define MAX_THREADS 200

static context thread_list[MAX_THREADS];
static __thread tid_t current_tid = NO_THREAD;
scheduler active_scheduler;
tid_t active_thread;
tid_t terminated_tid = NO_THREAD;

void print_register_state(const rfile *state) {
    if (state == NULL) {
        printf("Invalid register file pointer.\n");
        return;
    }

    printf("Register State:\n");
    printf("  RAX: %lu\n", state->rax);
    printf("  RBX: %lu\n", state->rbx);
    printf("  RCX: %lu\n", state->rcx);
    printf("  RDX: %lu\n", state->rdx);
    printf("  RSI: %lu\n", state->rsi);
    printf("  RDI: %lu\n", state->rdi);
    printf("  RBP: %lu\n", state->rbp);
    printf("  RSP: %lu\n", state->rsp);
    printf("  R8:  %lu\n", state->r8);
    printf("  R9:  %lu\n", state->r9);
    printf("  R10: %lu\n", state->r10);
    printf("  R11: %lu\n", state->r11);
    printf("  R12: %lu\n", state->r12);
    printf("  R13: %lu\n", state->r13);
    printf("  R14: %lu\n", state->r14);
    printf("  R15: %lu\n", state->r15);
}


tid_t lwp_create(lwpfun function_ptr, void *argument){
    if(!active_scheduler){
        active_scheduler = get_rr_schedule();
    }
    int i;
    /* put new thread into Thread List */
    for(i = 0; i<MAX_THREADS; i++){
        if(thread_list[i].tid == NO_THREAD){
            thread_list[i].tid = i +1;
            thread_list[i].stack = mmap(NULL, DEFAULT_STACK_SIZE, PROT_READ | PROT_WRITE, 
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            /* printf(HERE!! (lwp_create) \n)*/
            thread_list[i].stacksize = DEFAULT_STACK_SIZE;
            thread_list[i].status = LWP_LIVE;
            /* for scheduling, set to NULL for now */
            thread_list[i].sched_one = NULL;
            thread_list[i].sched_two = NULL;
            /* find bottom of stack */
            unsigned long *stack_base = (unsigned long *)thread_list[i].stack;
            /* point to top of stack */
            unsigned long *top = stack_base + (DEFAULT_STACK_SIZE / sizeof(unsigned long));
            /*printf("Initial top: %p\n", stack_base + DEFAULT_STACK_SIZE / sizeof(unsigned long));
            printf("Aligned top: %p\n", top);*/


            /* align with 16 byte boudary */
            top = (unsigned long *)((uintptr_t)top & ~(0xF));
            top--;
            /* return address @ top of stack */
            *(--top) = (unsigned long)lwp_exit;
            /* function ptr to address right below */
            *(--top) = (unsigned long) function_ptr;
            /*printf("Return address: %p\n", top + 1);
            printf("Function pointer: %p\n", top);*/

            thread_list[i].state.rsp = (unsigned long)top;
            thread_list[i].state.rdi = (unsigned long)argument;
            thread_list[i].state.rsi = (unsigned long)0;
            thread_list[i].state.rbp = (unsigned long)top;
            /*printf("rsp: %p\n", (void *)thread_list[i].state.rsp);
            printf("rdi: %p\n", (void *)thread_list[i].state.rdi);*/
            
            printf("admitting tid: %ld\n", thread_list[i].tid);
            /* admit new thread into scheduler */
            active_scheduler->admit(&thread_list[i]);

            return thread_list[i].tid;
        }
    }
    fprintf(stderr, "Max threads reached \n");
    return NO_THREAD;
}

void lwp_start(void){
    if(!active_scheduler){
        fprintf(stderr, "No active scheduler (lwp_start)\n");
        return;
    }
    lwp_yield();
}

void lwp_yield(void){
    context *next_thread = active_scheduler->next();
    if (!next_thread) {
    fprintf(stderr, "No next thread available.\n");
    return;
    }
    if(active_thread <= 0 || active_thread > MAX_THREADS){
        active_thread = next_thread->tid;
        next_thread = active_scheduler->next();
    }
    /* use swap rfile current w new */
    tid_t old = active_thread;
    tid_t new = next_thread->tid;
    /*printf("old id: %ld\n", old);
    printf("new id: %ld\n", new);

    printf("Old Register State:\n");
    print_register_state(&thread_list[old - 1].state);

    printf("New Register State:\n");
    print_register_state(&thread_list[new - 1].state);*/

    if(next_thread){
        active_thread = next_thread->tid;
        printf("Switching from thread %ld to thread %ld\n", old, new);
        swap_rfiles(&thread_list[old-1].state, &thread_list[new-1].state);
        printf("yaya!\n");
        /* if swapped thread hasnt finished, admit back into RR */
        if(thread_list[old-1].status == LWP_LIVE){
            active_scheduler->admit(&thread_list[old-1]);
        }
    }
}

void lwp_exit(int exitval){
    if(active_thread == NO_THREAD){
        printf("no thread (lwp_exit)\n");
        return;
    }
    context *current_thread = &thread_list[active_thread-1];
    current_thread->status = LWP_TERM;
    current_thread->exited = (thread)(unsigned long)exitval;

    if(current_thread->stack){
        munmap(current_thread->stack, current_thread->stacksize);
        current_thread->stack = NULL;
    }
    printf("HJERE!!\n");
    active_scheduler->remove(current_thread);
    current_thread->tid = NO_THREAD;

    context *next_thread = active_scheduler->next();
    if(next_thread){
        active_thread = next_thread->tid;
        swap_rfiles(NULL, &next_thread->state);
    }else{
        printf("Exit Exit\n");
        exit(EXIT_SUCCESS);
    }
}

tid_t lwp_wait(int *status){
    int i;
    /* go through threads and find terminated one then deallocate */
    for(i = 1; i < MAX_THREADS; i++){
        if(thread_list[i].tid != NO_THREAD && thread_list[i].status == LWP_TERM){
            terminated_tid = thread_list[i].tid;
            if(status != NULL){
                *status = (int) (unsigned long) thread_list[i].exited;
            }

            /* deallcated finished thread and make fields NULL */
            if(thread_list[i].stack){
                munmap(thread_list[i].stack, thread_list[i].stacksize);
                thread_list[i].stack = NULL;
            }
            /* marking the thread as finsihed */
            thread_list[i].tid = NO_THREAD;
            thread_list[i].status = 0;

            return terminated_tid;
        }
    }
    return NO_THREAD;
}


void lwp_set_scheduler(scheduler new_scheduler){
    active_scheduler = new_scheduler;
    if(active_scheduler && active_scheduler->init){
        active_scheduler->init();
    }
}

scheduler lwp_get_scheduler(void){
    return active_scheduler;
}


