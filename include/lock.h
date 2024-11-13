/* lock.h */

/* struct for different locks in Xinu based on the test_and_set hardware instruction. */

#define NSPINLOCKS 20   /* Maximum number of spinlocks that can be used	*/
#define NLOCKS 20       /* Maximum number of locks that can be used	*/
#define NALOCKS 20      /* Maximum number of active locks that can be used	*/
#define NPILOCKS 20     /* Maximum number of priority inversion locks that can be used	*/

typedef struct sl_lock_t
{
    uint32 flag;
}sl_lock_t;

typedef struct lock_t
{
    uint32 flag;
    uint32 guard;
    qid16 queue;
}lock_t;

typedef struct al_lock_t
{
    uint32 flag;
    uint32 guard;
    uint32 lock_id;
    qid16 queue;
}al_lock_t;

typedef struct pi_lock_t
{
    uint32 flag;
    uint32 guard;
    qid16 queue;
    pid32 curr_holder;
}pi_lock_t;

