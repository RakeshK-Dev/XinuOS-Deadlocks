/* lock.c - lock */

#include <xinu.h>
//#include "../include/lock.h"

/*------------------------------------------------------------------------
 *  lock  -  Initialization, lock and an unlock functions of lock_t
 *------------------------------------------------------------------------
 */

static uint32 lock_count = 0;

syscall initlock(lock_t *l) 
{
    if (lock_count >= NLOCKS)
    {
        return SYSERR;
    }
    lock_count++;
    l->flag = 0;  // Ensure the lock starts as unlocked
    l->guard = 0;
    l->queue = newqueue();
    return OK;
}

syscall lock(lock_t *l)
{
    while(test_and_set(&l->guard, 1)) 
    {
        sleepms(QUANTUM);
    }
    if (l->flag == 0) 
    {
        l->flag = 1;  // Acquire the lock
        l->guard = 0; // Release the guard
    }
    else 
    {
        enqueue(currpid, l->queue);  // Queue the current process
        setpark();  // Set park flag for the current process
        l->guard = 0; // Release the guard
        park();  // Park the current process
    }

    return OK;
}

syscall unlock(lock_t *l)
{
    while(test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (isempty(l->queue)) 
    {
        l->flag = 0;  // No processes waiting, release the lock
    }
    else 
    {
        unpark(dequeue(l->queue));  // Unpark the process and make it ready to run
    }

    l->guard = 0;  // Release the guard
    return OK;
}

syscall setpark() 
{
    intmask mask = disable();
    proctab[currpid].l_flag = TRUE;
    restore(mask);
    return OK;
}

syscall park() 
{
    intmask mask = disable();
    
    if (proctab[currpid].l_flag) 
    {
        proctab[currpid].prstate = PR_WAIT;
        resched();  // Yield the CPU to allow other processes to run
    }

    restore(mask);
    return OK;
}

syscall unpark(pid32 processid)
{
    intmask mask = disable();
    proctab[processid].prstate = PR_READY;
    insert(processid, readylist, proctab[processid].prprio);
    proctab[processid].l_flag = FALSE;
    restore(mask);

    return OK;
}
