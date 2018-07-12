/* This wraps the libnx semaphore for use with POSIX compatible code */

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H	1

#include <stddef.h>
#include <stdio.h>

namespace NX
{
  #include <switch.h>
}


__BEGIN_DECLS

typedef NX::Semaphore sem_t;

/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
int sem_init (sem_t *__sem, int __pshared, unsigned int __value);
/* Free resources associated with semaphore object SEM.  */
int sem_destroy (sem_t *__sem);

/* Open a named semaphore NAME with open flags OFLAG.  */
sem_t *sem_open (const char *__name, int __oflag, ...);

/* Close descriptor for named semaphore SEM.  */
int sem_close (sem_t *__sem);

/* Remove named semaphore NAME.  */
int sem_unlink (const char *__name);

/* Wait for SEM being posted.

   This function is a cancellation point and therefore not marked with
   __THROW.  */
int sem_wait (sem_t *__sem);

/* Test whether SEM is posted.  */
int sem_trywait (sem_t *__sem);

/* Post SEM.  */
int sem_post (sem_t *__sem);

/* Get current value of SEM and store it in *SVAL.  */
int sem_getvalue (sem_t *__restrict __sem, int *__restrict __sval);


__END_DECLS

#endif	/* semaphore.h */
