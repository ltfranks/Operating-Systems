#include "lwp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_THREADS 32
#define STACK_SIZE (1024 * 1024)  // Stack size per thread

static context threads[MAX_THREADS] = {0};
static __thread tid_t current_tid = NO_THREAD;  // Current thread ID
static scheduler current_scheduler = NULL;  // Current scheduler

void initialize_threads() {
    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].tid = NO_THREAD;
    }
}

tid_t lwp_create(lwpfun function_ptr, void *argument) {
    context *new_thread = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i].tid == NO_THREAD) {
            new_thread = &threads[i];
            new_thread->tid = i + 1;  // Assign thread ID
            break;
        }
    }

    if (!new_thread) {
        fprintf(stderr, "Maximum number of threads reached.\n");
        return NO_THREAD;
    }

    new_thread->stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (new_thread->stack == MAP_FAILED) {
        perror("Failed to allocate stack");
        return NO_THREAD;
    }

    new_thread->stacksize = STACK_SIZE;
    new_thread->status = LWP_LIVE;

    // Setup stack and registers
    unsigned long *stack_top = (unsigned long *)((char *)new_thread->stack + STACK_SIZE);
    stack_top = (unsigned long *)((uintptr_t)stack_top & -16);  // 16-byte align the stack top
    *--stack_top = (unsigned long)lwp_exit;  // Set up return address
    *--stack_top = (unsigned long)function_ptr;  // Function pointer

    new_thread->state.rsp = (unsigned long)stack_top;
    new_thread->state.rdi = (unsigned long)argument;

    if (current_scheduler && current_scheduler->admit) {
        current_scheduler->admit(new_thread);
    }

    return new_thread->tid;
}

void lwp_exit(void) {
    context *current_thread = &threads[current_tid - 1];
    munmap(current_thread->stack, current_thread->stacksize);
    current_thread->tid = NO_THREAD;  // Mark this thread slot as free
    lwp_schedule();
}

void lwp_schedule(void) {
    context *current_thread = &threads[current_tid - 1];
    context *next_thread = current_scheduler->next();

    if (next_thread) {
        current_tid = next_thread->tid;
        swap_rfiles(&current_thread->state, &next_thread->state);
    } else {
        fprintf(stderr, "No more threads to schedule.\n");
        exit(EXIT_FAILURE);
    }
}

void lwp_start(void) {
    if (!current_scheduler) {
        fprintf(stderr, "Scheduler not set.\n");
        return;
    }

    context *initial_thread = &threads[0];  // Start with the first thread
    current_tid = initial_thread->tid;
    current_scheduler->admit(initial_thread);
    lwp_schedule();
}
