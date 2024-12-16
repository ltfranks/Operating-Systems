#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>

static thread head = NULL;
static thread tail = NULL;
static int thread_count = 0;
/* initiaite circular linkedList */
void rr_init(void){
    head = NULL;
    tail = NULL;
    thread_count = 0;
}

/* lwp is done... clean up */
void rr_shutdown(void){
    head = NULL;
    tail = NULL;
    thread_count = 0;
}
/* sched_one is next */
/* sched_two is prev */
void rr_admit(thread new_thread){
    if(!head){
        head = new_thread;
        head->sched_one=head;
        head->sched_two=head;
    }else{
        thread last = head->sched_two;
        last->sched_one = new_thread;
        new_thread->sched_two = last;
        new_thread->sched_one=head;
        head->sched_two = new_thread;
        tail = new_thread;
    }
    thread_count++;

    if (head) {
        thread current = head;
        printf("Current Scheduler State:\n");
        do {
            printf("Thread ID: %ld, Status: %s\n", current->tid, 
                   (current->status == LWP_LIVE) ? "LIVE" : "OTHER");
            current = current->sched_one;
        } while (current != head);
    }
}

void rr_remove(thread thread_remove){
    if(!head || !thread_remove) return;
    
    thread current = head;

    do{
        if(current == thread_remove){
            /* only one thread left */
            if(current->sched_one == current){
                head = tail = NULL;
            } else{
                if(current == head){
                    head = current->sched_one; /* move head to next thread */
                }
                if(current == tail){
                    tail = current->sched_two; /* move tail to prev thread */
                }
                current->sched_two->sched_one = current->sched_one;
                current->sched_one->sched_two = current->sched_two;
            }
            printf("thread removed %p\n", thread_remove);

            thread_count--;
            break;
        }
        current = current->sched_one;
    } while (current != head);
}

thread rr_next(void) {
    if (!head) return NULL;
    thread next_thread = head;
    head = head->sched_one; // Move head to the next thread
    return next_thread;
}


int rr_qlen(void){
    return thread_count;
}

scheduler get_rr_schedule(void){
    static struct scheduler rr_scheduler = {
        .init = rr_init,
        .shutdown = rr_shutdown,
        .admit = rr_admit,
        .remove = rr_remove,
        .next = rr_next,
        .qlen = rr_qlen
    };
    return &rr_scheduler;
}
