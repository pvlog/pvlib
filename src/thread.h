#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include <semaphore.h>

#define THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

typedef pthread_t thread_t;
typedef pthread_mutex_t thread_mutex_t;
typedef sem_t thread_sem_t;
typedef pthread_cond_t thread_cond_t;

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg);
int thread_start(thread_t thread);
int thread_join(thread_t thread);

int thread_mutex_create(thread_mutex_t *mutex);
void thread_mutex_lock(thread_mutex_t *mutex);
void thread_mutex_unlock(thread_mutex_t *mutex);
void thread_mutex_destroy(thread_mutex_t *mutex);

int thread_cond_create(thread_cond_t *cond);
void thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex);
int thread_cond_timedwait(thread_cond_t *cond, thread_mutex_t *mutex, int ms);
void thread_cond_signal(thread_cond_t *cond);
void thread_cond_broadcast(thread_cond_t *cond);
void thread_cond_destroy(thread_cond_t *cond);

int thread_sem_create(thread_sem_t *sem, int value);
void thread_sem_release(thread_sem_t *sem);
void thread_sem_aquire(thread_sem_t *sem);
int thread_sem_timedaquire(thread_sem_t *sem, int ms);
void thread_sem_destroy(thread_sem_t *sem);

int thread_sleep(unsigned int mseconds);

/*
 #define thread_mutex_lock(mutex) \
	LOG_DEBUG("Lock"); \
	if (pthread_mutex_lock(mutex) < 0) { \
		LOG_ERROR("Failed locking mutex: "); \
	}


 #define thread_mutex_unlock(mutex) \
	LOG_DEBUG("UnLock"); \
	if (pthread_mutex_unlock(mutex) < 0) { \
		LOG_ERROR("Failed unlocking mutex: "); \
	}
 */

#endif /* THREAD_H */
