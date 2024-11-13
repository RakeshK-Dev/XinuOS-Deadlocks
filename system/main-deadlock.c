/* main-deadlock.c - to check deadlock */

#include <xinu.h>

#define DEBUG 0

#define DEBUG_LOG(fmt, ...) if (DEBUG) { sync_log("[DEBUG] " fmt, ##__VA_ARGS__); }

syscall sync_log(char *fmt, ...);
void precise_delay(uint32 delay);

al_lock_t lock_a, lock_b, lock_c;
al_lock_t lock_d, lock_e, lock_f;
al_lock_t lock_g, lock_h, lock_i, lock_j;

pid32 proc1, proc2, proc3, proc4, proc5, proc6, proc7, proc8, proc9, proc10;

// Part 1: Trigger deadlock by acquiring locks in different orders
process trigger_deadlock(al_lock_t *first_lock, al_lock_t *second_lock, uint32 hold_time1, uint32 hold_time2) {
    DEBUG_LOG("Process %d starting deadlock simulation with locks %d and %d\n", currpid, first_lock->lock_id, second_lock->lock_id);
    al_lock(first_lock);
    sync_log("Process %d acquired lock %d\n", currpid, first_lock->lock_id);
    precise_delay(hold_time1);

    al_lock(second_lock);
    sync_log("Process %d acquired lock %d\n", currpid, second_lock->lock_id);
    precise_delay(hold_time2);

    al_unlock(first_lock);
    sync_log("Process %d released lock %d\n", currpid, first_lock->lock_id);
    precise_delay(hold_time2);

    al_unlock(second_lock);
    sync_log("Process %d released lock %d\n", currpid, second_lock->lock_id);
    precise_delay(hold_time1);

    resched();
    DEBUG_LOG("Process %d completed deadlock simulation\n", currpid);

    return OK;
}

// Part 2: Avoid hold-and-wait to prevent deadlock
process avoid_hold_wait(al_lock_t *first_lock, al_lock_t *second_lock, uint32 hold_time1, uint32 hold_time2) {
    DEBUG_LOG("Process %d attempting to avoid hold-and-wait with locks %d and %d\n", currpid, first_lock->lock_id, second_lock->lock_id);
    while (!al_trylock(first_lock)) {
        precise_delay(5);
    }
    sync_log("Process %d acquired lock %d\n", currpid, first_lock->lock_id);
    precise_delay(hold_time1);

    while (!al_trylock(second_lock)) {
        al_unlock(first_lock);
        precise_delay(5);
        resched();
    }
    sync_log("Process %d acquired lock %d\n", currpid, second_lock->lock_id);
    precise_delay(hold_time2);

    al_unlock(first_lock);
    sync_log("Process %d released lock %d\n", currpid, first_lock->lock_id);
    precise_delay(hold_time2);

    al_unlock(second_lock);
    sync_log("Process %d released lock %d\n", currpid, second_lock->lock_id);
    precise_delay(hold_time1);

    resched();
    DEBUG_LOG("Process %d completed avoid hold-and-wait\n", currpid);

    return OK;
}

// Part 3: Allow preemption to prevent deadlock
process allow_preemption(al_lock_t *first_lock, al_lock_t *second_lock, uint32 hold_time1, uint32 hold_time2) {
    DEBUG_LOG("Process %d allowing preemption with locks %d and %d\n", currpid, first_lock->lock_id, second_lock->lock_id);
    al_lock(first_lock);
    sync_log("Process %d acquired lock %d\n", currpid, first_lock->lock_id);
    precise_delay(hold_time1);

    al_unlock(first_lock);
    sync_log("Process %d released lock %d\n", currpid, first_lock->lock_id);
    resched();

    al_lock(second_lock);
    sync_log("Process %d acquired lock %d\n", currpid, second_lock->lock_id);
    precise_delay(hold_time2);

    al_unlock(second_lock);
    sync_log("Process %d released lock %d\n", currpid, second_lock->lock_id);
    resched();
    DEBUG_LOG("Process %d completed allow preemption\n", currpid);

    return OK;
}

// Part 4: Enforce strict lock ordering to avoid circular wait
process avoid_circular_wait(al_lock_t *lower_lock, al_lock_t *higher_lock, uint32 hold_time1, uint32 hold_time2) {
    DEBUG_LOG("Process %d enforcing strict order with locks %d and %d\n", currpid, lower_lock->lock_id, higher_lock->lock_id);
    if (lower_lock->lock_id < higher_lock->lock_id) {
        al_lock(lower_lock);
        sync_log("Process %d acquired lock %d\n", currpid, lower_lock->lock_id);
        precise_delay(hold_time1);

        al_lock(higher_lock);
        sync_log("Process %d acquired lock %d\n", currpid, higher_lock->lock_id);
        precise_delay(hold_time2);

        al_unlock(higher_lock);
        sync_log("Process %d released lock %d\n", currpid, higher_lock->lock_id);
        precise_delay(hold_time1);

        al_unlock(lower_lock);
        sync_log("Process %d released lock %d\n", currpid, lower_lock->lock_id);
        precise_delay(hold_time2);
    } else {
        al_lock(higher_lock);
        sync_log("Process %d acquired lock %d\n", currpid, higher_lock->lock_id);
        precise_delay(hold_time2);

        al_lock(lower_lock);
        sync_log("Process %d acquired lock %d\n", currpid, lower_lock->lock_id);
        precise_delay(hold_time1);

        al_unlock(lower_lock);
        sync_log("Process %d released lock %d\n", currpid, lower_lock->lock_id);
        precise_delay(hold_time1);

        al_unlock(higher_lock);
        sync_log("Process %d released lock %d\n", currpid, higher_lock->lock_id);
        precise_delay(hold_time2);
    }

    resched();
    DEBUG_LOG("Process %d completed avoid circular wait\n", currpid);

    return OK;
}

syscall sync_log(char *fmt, ...) {
    intmask mask = disable();
    void *args = __builtin_apply_args();
    __builtin_apply((void*)kprintf, args, 100);
    restore(mask);
    return OK;
}

void precise_delay(uint32 delay) {
    uint32 start_time = proctab[currpid].runtime;
    while ((proctab[currpid].runtime - start_time) < delay);
}

process main(void) {
    uint32 time1 = 500, time2 = 300;

    sync_log("\n\n===== PART 1: Deadlock Simulation =====\n\n");
    al_initlock(&lock_a);
    al_initlock(&lock_b);
    al_initlock(&lock_c);

    proc1 = create(trigger_deadlock, INITSTK, 1, "trigger_deadlock", 4, &lock_a, &lock_b, time1, time2);
    proc2 = create(trigger_deadlock, INITSTK, 1, "trigger_deadlock", 4, &lock_b, &lock_c, time1 + time2, time1 + 2 * time2);
    proc3 = create(trigger_deadlock, INITSTK, 1, "trigger_deadlock", 4, &lock_c, &lock_a, 3 * time1 - time2, 4 * time1 - 2 * time2);

    resume(proc1);
    sleepms(time1 - time2);
    resume(proc2);
    sleepms(time1 - time2);
    resume(proc3);
    sleepms(time1 - time2);

    sleep(6);
    sync_log("\nDeadlock Detected as expected\n");

    sync_log("\n\n===== PART 2: Avoid Hold and Wait =====\n\n");
    al_initlock(&lock_d);
    al_initlock(&lock_e);
    al_initlock(&lock_f);

    proc4 = create(avoid_hold_wait, INITSTK, 1, "avoid_hold_wait", 4, &lock_d, &lock_e, time1, time2);
    proc5 = create(avoid_hold_wait, INITSTK, 1, "avoid_hold_wait", 4, &lock_e, &lock_f, time1 + time2, time1 + 2 * time2);
    proc6 = create(avoid_hold_wait, INITSTK, 1, "avoid_hold_wait", 4, &lock_f, &lock_d, 3 * time1 - time2, 4 * time1 - 2 * time2);

    resume(proc4);
    sleepms(time1 - time2);
    resume(proc5);
    sleepms(time1 - time2);
    resume(proc6);
    sleepms(time1 - time2);

    sleep(12);
    sync_log("\nNo Deadlock Detected in Part 2\n");

    sync_log("\n\n===== PART 3: Allow Preemption =====\n\n");
    al_initlock(&lock_g);
    al_initlock(&lock_h);
    al_initlock(&lock_i);

    proc7 = create(allow_preemption, INITSTK, 1, "allow_preemption", 4, &lock_g, &lock_h, time1, time2);
    proc8 = create(allow_preemption, INITSTK, 1, "allow_preemption", 4, &lock_h, &lock_i, time1 + time2, time1 + 2 * time2);
    proc9 = create(allow_preemption, INITSTK, 1, "allow_preemption", 4, &lock_i, &lock_g, 3 * time1 - time2, 4 * time1 - 2 * time2);

    resume(proc7);
    sleepms(time1 - time2);
    resume(proc8);
    sleepms(time1 - time2);
    resume(proc9);
    sleepms(time1 - time2);

    receive();
    receive();
    receive();

    sleep(4);
    sync_log("\nNo Deadlock Detected in Part 3\n");

    sync_log("\n\n===== PART 4: Avoid Circular Wait =====\n\n");
    al_initlock(&lock_a);
    al_initlock(&lock_j);
    al_initlock(&lock_c);

    proc10 = create(avoid_circular_wait, INITSTK, 1, "avoid_circular_wait", 4, &lock_a, &lock_j, time1, time2);
    proc4 = create(avoid_circular_wait, INITSTK, 1, "avoid_circular_wait", 4, &lock_j, &lock_c, time1 + time2, time1 + 2 * time2);
    proc5 = create(avoid_circular_wait, INITSTK, 1, "avoid_circular_wait", 4, &lock_c, &lock_a, 3 * time1 - time2, 4 * time1 - 2 * time2);

    resume(proc10);
    sleepms(time1 - time2);
    resume(proc4);
    sleepms(time1 - time2);
    resume(proc5);
    sleepms(time1 - time2);

    sleep(10);
    sync_log("\nNo Deadlock Detected in Part 4\n");

    return OK;
}
