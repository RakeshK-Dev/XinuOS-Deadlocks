/* active_lock.c - active_lock */

#include <xinu.h>
//#include "../include/lock.h"

/*------------------------------------------------------------------------
 *  active_lock  -  Initialization, lock, and unlock functions of al_lock_t
 *------------------------------------------------------------------------
 */

#define DEBUG_MODE 0

#define DEBUG_PRINT(fmt, ...) \
            do { if (DEBUG_MODE) kprintf(fmt, ##__VA_ARGS__); } while (0)

static int al_count = 0;

uint32 locks[NALOCKS] = {-1};

void check_deadlock(pid32 curr_process, al_lock_t *l)
{
    int involved_processes[NALOCKS] = {-1};
    int curr_lock_id = l->lock_id;
    int iterator = curr_process;
    int process_count = 0;

    DEBUG_PRINT("Debug: Checking for deadlocks with process %d on lock %d\n", curr_process, l->lock_id);

    while ((curr_lock_id != -1) && (iterator != -1))
    {
        involved_processes[process_count++] = iterator;
        iterator = locks[curr_lock_id];

        if (iterator == involved_processes[0])  // Deadlock detected
        {
            DEBUG_PRINT("Debug: Deadlock detected in process chain\n");

            int i , j;
            // Sort the process chain
            for (i = 0; i < process_count; ++i)
            {
                for (j = i + 1; j < process_count; ++j)
                {
                    if (involved_processes[i] > involved_processes[j])
                    {
                        involved_processes[i] ^= involved_processes[j];
                        involved_processes[j] ^= involved_processes[i];
                        involved_processes[i] ^= involved_processes[j];
                    }
                }
                /*int key = involved_processes[i];
                int k = i - 1;

                // Shift elements that are greater than `key` to one position ahead
                while (k >= 0 && involved_processes[k] > key)
                {
                    involved_processes[k + 1] = involved_processes[k];
                    k = k - 1;
                }
                involved_processes[k + 1] = key;*/
            }

            // Print deadlock chain
            kprintf("deadlock_detected=");
            for (j = 0; j < process_count; j++)
            {
                kprintf("P%d", involved_processes[j]);
                if (j != (process_count - 1))
                {
                    kprintf("-");
                }
            }
            kprintf("\n");
            break;
        }

        curr_lock_id = proctab[iterator].pendingLockId;
    }
}

syscall al_initlock(al_lock_t *l)
{
    if (al_count >= NALOCKS)
    {
        DEBUG_PRINT("Debug: Failed to initialize lock, max locks reached\n");
        return SYSERR;
    }

    l->flag = 0;
    l->guard = 0;
    l->lock_id = al_count++;
    l->queue = newqueue();

    DEBUG_PRINT("Debug: Initialized lock with ID %d\n", l->lock_id);

    return OK;
}

syscall al_lock(al_lock_t *l)
{
    DEBUG_PRINT("Debug: Process %d attempting to acquire lock %d\n", currpid, l->lock_id);

    while (test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (l->flag == 0) 
    {
        l->flag = 1;
        l->guard = 0;
        locks[l->lock_id] = currpid;

        DEBUG_PRINT("Debug: Process %d acquired lock %d\n", currpid, l->lock_id);
    }
    else 
    {
        proctab[currpid].pendingLockId = l->lock_id;
        check_deadlock(currpid, l);
        enqueue(currpid, l->queue);
        al_setpark();
        l->guard = 0;
        al_park();

        DEBUG_PRINT("Debug: Process %d parked and waiting for lock %d\n", currpid, l->lock_id);
    }

    return OK;
}

syscall al_unlock(al_lock_t *l)
{
    DEBUG_PRINT("Debug: Process %d attempting to release lock %d\n", currpid, l->lock_id);

    while (test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (isempty(l->queue)) 
    {
        l->flag = 0;
        locks[l->lock_id] = -1;
        DEBUG_PRINT("Debug: Lock %d released with no waiting processes\n", l->lock_id);
    }
    else 
    {
        pid32 processid = dequeue(l->queue);
        proctab[processid].pendingLockId = -1;
        locks[l->lock_id] = processid;
        al_unpark(processid);

        DEBUG_PRINT("Debug: Lock %d passed to process %d\n", l->lock_id, processid);
    }

    proctab[currpid].pendingLockId = -1;
    l->guard = 0;
    return OK;
}

bool8 al_trylock(al_lock_t *l)
{
    DEBUG_PRINT("Debug: Process %d trying to acquire lock %d without waiting\n", currpid, l->lock_id);

    while (test_and_set(&l->guard, 1))
    {
        sleepms(QUANTUM);
    }

    if (l->flag == 0) 
    {
        l->flag = 1;
        l->guard = 0;
        locks[l->lock_id] = currpid;
        DEBUG_PRINT("Debug: Process %d successfully acquired lock %d\n", currpid, l->lock_id);
        return TRUE;
    }

    l->guard = 0;
    DEBUG_PRINT("Debug: Lock %d is already held, process %d could not acquire it\n", l->lock_id, currpid);
    return FALSE;
}

syscall al_setpark() 
{
    intmask mask = disable();
    proctab[currpid].l_flag = TRUE;
    restore(mask);

    DEBUG_PRINT("Debug: Process %d set to park\n", currpid);

    return OK;
}

syscall al_park()
{
    intmask mask = disable();

    if (proctab[currpid].l_flag == TRUE) 
    {
        proctab[currpid].prstate = PR_WAIT;
        resched();
    }

    restore(mask);

    DEBUG_PRINT("Debug: Process %d is parked\n", currpid);

    return OK;
}

syscall al_unpark(pid32 processid)
{
    intmask mask = disable();
    proctab[processid].prstate = PR_READY;
    insert(processid, readylist, proctab[processid].prprio);
    proctab[processid].l_flag = FALSE;
    restore(mask);

    DEBUG_PRINT("Debug: Process %d unparked and set to ready\n", processid);

    return OK;
}
