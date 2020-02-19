#ifndef _ROLL_BUFFER_H_
#define _ROLL_BUFFER_H_

#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netdb.h>
#include <dlfcn.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <mntent.h>
#include <getopt.h>
#include <stdint.h>
/* timeout options */
#define SEM_EMPTY 0
#define SEM_FULL  1
#define NO_WAIT	0
#define WAIT_FOREVER (-1)

#define DEFAULT_BUCKET_NUM 50
/*
 * Function: semOpen
 * Description: initialize and open a named semaphore
 * Input:   name - points to a string naming a semaphore object
 *          oflag - O_CREAT,O_EXCL
 * Output:  none
 * Return:  the address of the semaphore if successful,otherwise
 *          return a value of SEM_FAILED and set errno to indicate the error
 *
 */
inline  sem_t *semOpen(const char *name, int oflag, ...)
{
    sem_t *rval;
    va_list args;

    va_start(args, oflag);
    rval = sem_open(name, oflag, args);
    va_end(args);

    return rval;
}

/*
 * Function: semPost
 * Description: unlock the semaphore
 * Input:   sem - points to the semaphore
 * Output:  none
 * Return:  0 if successful,otherwise return -1 and set errno
 *
 */
inline  int semPost(sem_t *sem)
{
    int rval;

    rval = sem_post(sem);

    return rval;
}

/*
 * Function: semWait
 * Description: lock the semaphore
 * Input:   sem - points to the semaphore
 *          wait_ms - NO_WAIT,WAIT_FOREVER,or max wait time(ms)
 * Output:  none
 * Return:  0 if successful,otherwise return -1 and set errno
 *
 */
inline int semWait(sem_t *sem, int wait_ms)
{
    int rval = -1;
    struct timeval now;
    struct timespec timeout;
    long sec, usec;

    if (wait_ms == NO_WAIT)
    {
        rval = sem_trywait(sem);
    }
    else if (wait_ms == WAIT_FOREVER)
    {
        rval = sem_wait(sem);
    }
    else
    {
        sec = 0;
        usec = wait_ms * 1000;
        gettimeofday(&now, (struct timezone *)NULL);
        usec += now.tv_usec;
        if (usec > 1000000)
        {
            sec = usec / 1000000;
            usec = usec % 1000000;
        }
        timeout.tv_sec = now.tv_sec + sec;
        timeout.tv_nsec = usec * 1000;
        rval = sem_timedwait(sem, &timeout);
    }

    return rval;
}
/*
 * Function: semInit
 * Description: initialize a unnamed semaphore
 * Input:   value -  the value of the initialized semaphore.
 * Output:  sem - semaphore
 * Return:  if fail, return -1, and set errno to indicate the error
 *			if success, return zero.
 *
 */
inline int semInit(sem_t *sem, unsigned value)
{
    int rval;

    /* the semaphore is shared between threads of the process */
    rval = sem_init(sem, 0, value);

    if (rval != -1)
    {
        return 0;
    }

    return rval;
}


template < class bucket>
class CRollBuffer
{
public:

    CRollBuffer()
    {
        in = 0;
        out = 0;
        m_lTimeSlice =0;
        m_bucketNum=0;
        printf("CRollBuffer bucketNum %d\n",m_bucketNum);
    }
    CRollBuffer(uint64_t lTimeSlice)
    {
        in = 0;
        out = 0;
        m_lTimeSlice =lTimeSlice;
    }
    ~CRollBuffer()
    {
        if(pBuffNode != NULL)
        {
            for(int index=0 ;index<m_bucketNum ;index++)
            {

                if(pBuffNode[index].pbucket != NULL)
                {
                    pBuffNode[index].pbucket->~bucket_type();
                    //printf("~CRollBuffer index %d\n",index);
                    delete pBuffNode[index].pbucket;
                }

            }
            delete pBuffNode;
        }

    }
    typedef bucket bucket_type;
    struct sBuffNode
    {
        sBuffNode()
        {
            semInit(&struSemFull, SEM_EMPTY);
            semInit(&struSemEmpty, SEM_FULL);
        }
        bucket_type *pbucket;
        sem_t       struSemEmpty;
        sem_t      struSemFull; /*has data in buffer*/
    };


    int Init(int bucketNum)
    {
        m_bucketNum = bucketNum;

        pBuffNode = new sBuffNode[bucketNum];

        for(int index=0 ;index<bucketNum ;index++)
        {

            pBuffNode[index].pbucket = new bucket_type();

            semInit(&(pBuffNode[index].struSemFull), SEM_EMPTY);
            semInit(&(pBuffNode[index].struSemEmpty), SEM_FULL);
        }
    }

    int Init(int bucketNum,uint64_t lTimeSlice)
    {
        m_bucketNum = bucketNum;

        pBuffNode = new sBuffNode[bucketNum];

        for(int index=0 ;index<bucketNum ;index++)
        {

            pBuffNode[index].pbucket = new bucket_type(lTimeSlice);

            semInit(&(pBuffNode[index].struSemFull), SEM_EMPTY);
            semInit(&(pBuffNode[index].struSemEmpty), SEM_FULL);
        }
    }
    CRollBuffer::bucket_type *getReadBucket(int timeout )
    {
        if(semWait(&(pBuffNode[out].struSemFull), timeout) != 0)
        {
            // printf("buff %d is not Full\n",out);
            return NULL;
        }
        return pBuffNode[out].pbucket;
    }

    void undoGetReadBucket() {
        sem_post(& pBuffNode[out].struSemFull);
    }

    CRollBuffer::bucket_type *getWriteBucket(int timeout)
    {
        if(semWait(&(pBuffNode[in].struSemEmpty), timeout) != 0)
        {
            return NULL;
        }
        return pBuffNode[in].pbucket;
    }
    void undoGetWriteBucket() {
        sem_post(& pBuffNode[in].struSemEmpty);
    }

    CRollBuffer::bucket_type * back()
    {
        return pBuffNode[in].pbucket;
    }
    int push_back()
    {
        in= (1+in)%m_bucketNum;
        //if(in % m_bucketNum==0)
        //{
        //in = 0;

        pBuffNode[in].pbucket->~bucket_type();
        //}

        //printf("push_back buff %d is full ,total %d\n",in,m_bucketNum);

        return 0;
    }
    int Write()
    {
        if(semPost(&(pBuffNode[in].struSemFull)) != 0)
        {
            return -1;
        }

        //printf("buff %d is full ,total %d\n",in,m_bucketNum);
        in= (1+in)%m_bucketNum;
        return in;

    }
    int Read()
    {
        if(semPost(&(pBuffNode[out].struSemEmpty)) != 0)
        {
            return -1;
        }
        //printf("buff %d is Empty\n",out);
        out= (1+out)%m_bucketNum;
        return out;
    }
    sBuffNode *pBuffNode;
public:
    uint64_t m_lTimeSlice;
    int setBuffFull();
    int setBuffEmpty();
    int in ;
    int out;
    int m_bucketNum;

};
#endif
