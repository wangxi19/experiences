#ifndef MSGQ_H
#define MSGQ_H
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>

//m is a power of 2
#define mod(n, m) ((n) & ((m) - 1))

typedef struct msg_q_s
{
    const char* name;
    void* addr;
    unsigned int nelts;
    unsigned int capacity;
    unsigned int szelt;

    atomic_uint celt_head;
    unsigned int celt_tail;
    atomic_uint pelt_head;
    unsigned int pelt_tail;
}msg_q;

#define	compiler_barrier() do {		\
    asm volatile ("" : : : "memory");	\
    } while(0)

#ifndef smp_rmb
#define smp_rmb() compiler_barrier()
#endif

#ifndef smp_wmb
#define smp_wmb() compiler_barrier()
#endif

static inline void do_pause(void)
{
    compiler_barrier();
}

static void* create_shared_memory(const char* name, unsigned int size) {
    void* retval;
    int fd = shm_open(name, O_RDWR|O_CREAT, 0666);
    if (fd == -1)
    {
        perror("shm_open");
        return NULL;
    }

    if (ftruncate(fd, size) == -1)
    {
        close(fd);
        perror("ftruncate");
        return NULL;
    }

    retval = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    return retval;
}

static void* get_shared_memory(const char* name, unsigned int size) {
    int sfd = shm_open(name, O_RDWR|O_CREAT, 0666);
    if (sfd == -1)
    {
        perror("shm_open");
        return NULL;
    }

    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, sfd, 0);
}

static inline void destroy_shared_memory(const char* name, void* addr, size_t sz)
{
    munmap(addr, sz);
    shm_unlink(name);
}

static void reset_msg_queue(msg_q* msgq)
{
    //    msgq->name = NULL;
    //    msgq->addr = NULL;
    //    msgq->nelts = 0;
    //    msgq->szelt = 0;
    msgq->celt_head = 0;
    msgq->celt_tail = 0;
    msgq->pelt_head = 0;
    msgq->pelt_tail = 0;
}

/**
 * @brief create_msg_queue
 * @param name
 * @param nelts
 * must is a power of 2
 * @param szelt
 * @param out
 * @return
 */
static int create_msg_queue(const char* name, unsigned int nelts, unsigned int szelt, msg_q** out)
{
    *out = create_shared_memory(name, sizeof(msg_q) + nelts * sizeof(void*) + nelts * szelt);
    if (*out == (void *) -1)
    {
        perror("mmap");
        return -1;
    }
    (*out)->name = name;
    (*out)->addr = (char*)(*out) + sizeof(msg_q);
    (*out)->nelts = nelts;
    (*out)->capacity = nelts - 1;
    (*out)->szelt = szelt;
    (*out)->celt_head = 0;
    (*out)->celt_tail = 0;
    (*out)->pelt_head = 0;
    (*out)->pelt_tail = 0;
    return 0;
}

static int get_msg_queue(const char* name, unsigned int nelts, unsigned int szelt, msg_q** out)
{
    *out = get_shared_memory(name, nelts * szelt + sizeof(msg_q));
    if (*out == (void *) -1)
    {
        perror("mmap");
        return -1;
    }
    return 0;
}

static int msg_enqueue_mp(msg_q* msgq, const void* msg)
{
    int success;
    unsigned int oi;
    unsigned int ni;
    unsigned int capacity = msgq->capacity;
    unsigned int free_entries;
    /* **update head
     *compete in multi-producer
    */
    do {
        oi = msgq->pelt_head;
        smp_rmb();
        free_entries = capacity + msgq->celt_tail - oi;
        if (free_entries == 0)
            return -1;

        ni = oi + 1;
        success = atomic_compare_exchange_strong(&msgq->pelt_head, &oi, ni);
    } while (success == 0);
    //do enqueue
    memcpy((char*)msgq->addr + mod(oi, msgq->nelts) * msgq->szelt, msg, msgq->szelt);

    smp_wmb();
    /* **update tail
    * If there are other enqueues/dequeues in progress that preceded us,
    * we need to wait for them to complete
    */
    while ((msgq->pelt_tail != oi)) {do_pause();}
    msgq->pelt_tail = ni;

    return 0;
}

static int msg_dequeue_mp(msg_q* msgq, void* msg)
{
    int success;
    unsigned int oi;
    unsigned int ni;
    unsigned int free_entries;

    /* **update head
     *compete in multi-consumer
    */
    do {
        oi = msgq->celt_head;
        /* add rmb barrier to avoid load/load reorder in weak
        * memory model. It is noop on x86
        */
        smp_rmb();
        free_entries = msgq->pelt_tail - oi;
        //if empty
        if (free_entries == 0)
            return -1;

        ni = oi + 1;
        success = atomic_compare_exchange_strong(&msgq->celt_head, &oi, ni);
    } while (success == 0);
    //do dequeue
    memcpy(msg, (char*)msgq->addr + mod(oi, msgq->nelts) * msgq->szelt, msgq->szelt);
    smp_rmb();
    /* **update tail
    * If there are other enqueues/dequeues in progress that preceded us,
    * we need to wait for them to complete
    */
    while ((msgq->celt_tail != oi)) {do_pause();}
    msgq->celt_tail = ni;

    return 0;
}

static inline void destroy_msg_queue(const char* name, msg_q* msgq)
{
    destroy_shared_memory(name, msgq, msgq->nelts * msgq->szelt);
}

#endif // MSGQ_H
