/* This wraps the libnx semaphore for use with POSIX compatible code */

#include "semaphore.h"

/* Initialize semaphore object SEM to VALUE.  If PSHARED then share it
   with other processes.  */
int sem_init (sem_t *__sem, int __pshared, unsigned int __value) {
  // Ignore PSHARED, I don't think the Switch can even do that
  NX::semaphoreInit(__sem, __value);
  return 0; // Success
}

/* Free resources associated with semaphore object SEM.  */
int sem_destroy (sem_t *__sem) {
  delete __sem;
  return 0;
}

/* Open a named semaphore NAME with open flags OFLAG.  */
sem_t *sem_open (const char *__name, int __oflag, ...) {
  // Ignore name and OFLAG
  sem_t *ret = new sem_t;
  return ret;
}

/* Close descriptor for named semaphore SEM.  */
int sem_close (sem_t *__sem) {
  return 0; // NOOP, probably
}

/* Remove named semaphore NAME.  */
int sem_unlink (const char *__name) {
  return -1; // Just error, we don't do named semaphores
}

/* Wait for SEM being posted.
   This function is a cancellation point and therefore not marked with
   __THROW.  */
int sem_wait (sem_t *__sem) {
  NX::semaphoreWait(__sem);
  return 0;
}

/* Test whether SEM is posted.  */
int sem_trywait (sem_t *__sem) {
  return NX::semaphoreTryWait(__sem) ? 0 : -1;
}

/* Post SEM.  */
int sem_post (sem_t *__sem) {
  NX::semaphoreSignal(__sem);
  return 0;
}

/* Get current value of SEM and store it in *SVAL.  */
int sem_getvalue (sem_t *__restrict __sem, int *__restrict __sval) {
  return __sem->count;
}


