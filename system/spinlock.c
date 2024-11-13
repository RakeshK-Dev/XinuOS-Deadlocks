/* spinlock.c - spinlock */

#include <xinu.h>
//#include "../include/lock.h"

/*------------------------------------------------------------------------
 *  spinlock  -  Initialization, lock and an unlock functions of sl_lock_t
 *------------------------------------------------------------------------
 */

static uint32 sl_lock_count = 0;

syscall sl_initlock(sl_lock_t *l)
{
    if (sl_lock_count >= NSPINLOCKS)
    {
        return SYSERR;
    }
    sl_lock_count++;
    l->flag = 0;  // Ensure lock is unlocked initially
    return OK;
}

syscall sl_lock(sl_lock_t *l)
{
    while (test_and_set(&l->flag, 1));
    return OK;
}

syscall sl_unlock(sl_lock_t *l)
{    
    l->flag = 0;  // Release the lock
    return OK;
}
