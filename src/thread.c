#define _XOPEN_SOURCE 600
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>

#include "thread.h"
#include "log.h"

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
	int ret;

	ret = pthread_create(thread, NULL, start_routine, arg);
	if (ret != 0) {
		LOG_ERROR("Failed starting thread: %s", strerror(ret));
		return -1;
	}

	return 0;

}

int thread_join(thread_t thread)
{
	int ret;

	ret = pthread_join(thread, NULL);
	if (ret != 0) {
		LOG_ERROR("Failed joining thread: %s", strerror(ret));
		return -1;
	}
	return 0;
}

int thread_mutex_create(thread_mutex_t *mutex)
{
	int ret;
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

	ret = pthread_mutex_init(mutex, &attr);
	if (ret != 0) {
		LOG_ERROR("Failed creating mutex: %s", strerror(ret));
		return -1;
	}

	return 0;
}

void thread_mutex_lock(thread_mutex_t *mutex)
{
	int ret;

	ret = pthread_mutex_lock(mutex);
	if (ret != 0) {
		LOG_ERROR("Failed locking mutex: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

void thread_mutex_unlock(thread_mutex_t *mutex)
{
	int ret;

	ret = pthread_mutex_unlock(mutex);
	if (ret != 0) {
		fprintf(stderr, "Failed unlocking mutex: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

void thread_mutex_destroy(thread_mutex_t *mutex)
{
	int ret;

	ret = pthread_mutex_destroy(mutex);
	if (ret != 0) {
		LOG_ERROR("Failed destroying mutex: %s", strerror(ret));
	}
}

int thread_cond_create(thread_cond_t *cond)
{
	int ret;

	ret = pthread_cond_init(cond, NULL);
	if (ret != 0) {
		LOG_ERROR("Failed creating condition: %s", strerror(ret));
		return -1;
	}

	return 0;
}

void thread_cond_wait(thread_cond_t *cond, thread_mutex_t *mutex)
{
	int ret;

	ret = pthread_cond_wait(cond, mutex);
	if (ret != 0) {
		LOG_ERROR("Error waiting for condition: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

int thread_cond_timedwait(thread_cond_t *cond, thread_mutex_t *mutex, int ms)
{
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);

	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (ms % 1000) * 1000;

	ret = pthread_cond_timedwait(cond, mutex, &ts);
	if (ret != 0) {
		if (ret != ETIMEDOUT) {
			LOG_ERROR("Error waiting for condition: %s", strerror(ret));
		}
		return -1;
	}

	return 0;

}

void thread_cond_signal(thread_cond_t *cond)
{
	int ret;

	ret = pthread_cond_signal(cond);
	if (ret != 0) {
		LOG_ERROR("Error signaling condition: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

void thread_cond_broadcast(thread_cond_t *cond)
{
	int ret;

	ret = pthread_cond_broadcast(cond);
	if (ret != 0) {
		LOG_ERROR("Error broadcasting condition: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

void thread_cond_destroy(thread_cond_t *cond)
{
	int ret;

	ret = pthread_cond_destroy(cond);
	if (ret != 0) {
		LOG_ERROR("Failed destroying condition: %s", strerror(ret));
	}
}

int thread_sem_create(thread_sem_t *sem, int value)
{
	int ret;

	ret = sem_init(sem, 0, value);
	if (ret != 0) {
		LOG_ERROR("Failed creating sem: %s", strerror(ret));
		return -1;
	}

	return 0;
}

void thread_sem_release(thread_sem_t *sem)
{
	int ret;

	ret = sem_post(sem);
	if (ret != 0) {
		LOG_ERROR("Error realising semaphore: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

void thread_sem_aquire(thread_sem_t *sem)
{
	int ret;

	ret = sem_wait(sem);
	if (ret != 0) {
		LOG_ERROR("Error waiting for semaphore: %s", strerror(ret));
		exit(EXIT_FAILURE);
	}
}

int thread_sem_timedaquire(thread_sem_t *sem, int ms)
{
	struct timespec ts;
	int ret;

	clock_gettime(CLOCK_REALTIME, &ts);

	ts.tv_sec += ms / 1000;
	ts.tv_nsec += (ms % 1000) * 1000;

	ret = sem_timedwait(sem, &ts);
	if (ret != 0) {
		if (errno != ETIMEDOUT) {
			LOG_ERROR("Error waiting for semaphore: %s", strerror(errno));
		}
		return -1;
	}

	return 0;
}

void thread_sem_destroy(thread_sem_t *sem)
{
	int ret;

	ret = sem_destroy(sem);
	if (ret != 0) {
		LOG_ERROR("Failed destroying semaphore: %s", strerror(ret));
	}
}
