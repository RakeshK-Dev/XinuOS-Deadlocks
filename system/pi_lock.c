/* pi_lock.c - pi_lock */

#include <xinu.h>
//#include "../include/lock.h"

/*------------------------------------------------------------------------
 *  pi_lock  -  Lock implementation to avoid priority inversion
 *------------------------------------------------------------------------
 */

#define DEBUG_MODE 0  // Set to 1 to enable debug messages, 0 to disable

#define DEBUG_PRINT(fmt, ...) \
            do { if (DEBUG_MODE) kprintf(fmt, ##__VA_ARGS__); } while (0)

static int pi_count = 0;

// Initialize a priority-inheritance lock
syscall pi_initlock(pi_lock_t *l) 
{
    if (pi_count == NPILOCKS)
    {
        DEBUG_PRINT("Debug: Failed to initialize lock, max locks reached\n");
        return SYSERR;
    }

    l->flag = 0;
    l->curr_holder = 0;
    l->guard = 0;
    l->queue = newqueue();
    pi_count++;

    DEBUG_PRINT("Debug: Initialized priority inheritance lock\n");
    return OK;
}

// Acquire the lock if available or wait if it is already held
syscall pi_lock(pi_lock_t *l)
{
    DEBUG_PRINT("Debug: Process %d attempting to acquire lock\n", currpid);

    while (test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (l->flag == 0)
    {
        l->flag = 1;
        l->guard = 0;
        l->curr_holder = currpid;

        DEBUG_PRINT("Debug: Process %d acquired lock\n", currpid);
    }
    else
    {
        DEBUG_PRINT("Debug: Lock held, process %d waiting\n", currpid);
        enqueue(currpid, l->queue);
        pi_setpark();
        l->guard = 0;
        pi_park(l);
    }

    return OK;
}

// Release the lock and unpark the next waiting process, if any
syscall pi_unlock(pi_lock_t *l)
{
    DEBUG_PRINT("Debug: Process %d attempting to release lock\n", currpid);

    while (test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (isempty(l->queue))
    {
        l->flag = 0;
        l->guard = 0;
        DEBUG_PRINT("Debug: Lock released with no waiting processes\n");
    }
    else
    {
        DEBUG_PRINT("Debug: Lock held by waiting processes, unpark next\n");
        pi_unpark(l);
    }

    return OK;
}

// update priorities between the current lock owner and current process
void update_priority(pi_lock_t *l) 
{
    DEBUG_PRINT("Debug: Updating priority for lock owner %d based on process %d\n", l->curr_holder, currpid);

    if (proctab[currpid].prprio > proctab[l->curr_holder].prprio) 
    {
        qid16 iterator = firstid(readylist);
        qid16 ready_tail = queuetail(readylist);

        kprintf("priority_change=P%d::%d-%d\n", l->curr_holder, proctab[l->curr_holder].prprio, proctab[currpid].prprio);
        if (proctab[l->curr_holder].priority == 0) 
        {
            proctab[l->curr_holder].priority = proctab[l->curr_holder].prprio;
        } 
        proctab[l->curr_holder].prprio = proctab[currpid].prprio;

        while (iterator != ready_tail) 
        {
            if (iterator == l->curr_holder) 
            {
                getitem(l->curr_holder);
                insert(l->curr_holder, readylist, proctab[l->curr_holder].prprio);
                DEBUG_PRINT("Debug: Lock owner %d priority updated in readylist\n", l->curr_holder);
            }
            iterator = queuetab[iterator].qnext;
        }
    }
}

// Restore priority inheritance for processes waiting on the lock
void restore_inheritance(pi_lock_t *l) 
{
    pri16 saved_priority = proctab[currpid].priority;
    DEBUG_PRINT("Debug: Restoring priority inheritance for process %d\n", currpid);

    if (proctab[currpid].priority > 0) 
    {
        int i;
        for (i = 0; i < NPROC; i++) 
        {
            if (proctab[i].pendingLock->curr_holder == currpid && 
                proctab[i].prprio > saved_priority && 
                proctab[i].pendingLock != l) 
            {
                saved_priority = proctab[i].prprio;
            }
        }

        kprintf("priority_change=P%d::%d-%d\n", currpid, proctab[currpid].prprio, saved_priority);
        proctab[currpid].prprio = saved_priority;

        if (saved_priority == proctab[currpid].priority) 
        {
            proctab[currpid].priority = 0;
        }
        DEBUG_PRINT("Debug: Process %d priority restored to %d\n", currpid, proctab[currpid].prprio);
    }
}

// Set parking flag for a process
syscall pi_setpark() 
{
    intmask mask = disable();
    proctab[currpid].l_flag = TRUE;
    DEBUG_PRINT("Debug: Process %d set to park\n", currpid);
    restore(mask);
    return OK;
}

// Park the current process and update priority if necessary
syscall pi_park(pi_lock_t *l)
{
    intmask mask = disable();

    if (proctab[currpid].l_flag == TRUE) 
    {
        proctab[currpid].prstate = PR_WAIT;
        proctab[currpid].pendingLock = l;
        update_priority(l);
        resched();
        DEBUG_PRINT("Debug: Process %d parked and waiting for lock\n", currpid);
    }

    restore(mask);
    return OK;
}

// Unpark the next process waiting on the lock and restore its priority
syscall pi_unpark(pi_lock_t *l)
{
    intmask mask = disable();
    pid32 next_process = dequeue(l->queue);
    l->curr_holder = next_process;
    proctab[next_process].pendingLock = NULL;

    pri16 current_priority = proctab[currpid].prprio;
    restore_inheritance(l);

    if (proctab[next_process].prprio != current_priority)
    {
        kprintf("priority_change=P%d::%d-%d\n", next_process, proctab[next_process].prprio, current_priority);
        proctab[next_process].priority = proctab[next_process].prprio;
        proctab[next_process].prprio = current_priority;
    }

    proctab[next_process].l_flag = FALSE;
    proctab[next_process].prstate = PR_READY;
    insert(next_process, readylist, proctab[next_process].prprio);
    l->guard = 0;

    DEBUG_PRINT("Debug: Process %d unparked and set to ready\n", next_process);

    restore(mask);
    return OK;
}
