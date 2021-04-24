#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "so_scheduler.h"
#include "prio_queue.h"
#include "utils.h"

/* Structure for keeping info about a thread */
typedef struct threadStruct
{
	tid_t tid;
	/* function to be executed after creation */
	so_handler *func;
	uint8_t priority;
	/* time remaining to run */
	uint32_t timer;
	/* used for waiting / signaling on io */
	unsigned int io;
	/* pointer to value used in pthread_key_t structure */
	unsigned int *v_addr;
	sem_t *sem;
} tStruct;

typedef struct Scheduler {
	/* max number of event no */
	unsigned int io_ev_n;
	/* quantum for each thread */
	unsigned int quantum;
	/* key used for thread specific data */
	pthread_key_t key;
	/* array for storing info about each thread */
	tStruct *tArr;
	/* current array size */
	size_t arr_size;
	/* max allocated array size */
	size_t arr_max_size;
	/* head of priority queue used for thread execution oreder */
	pqNode *pq_head;
} Scheduler;

static Scheduler tScheduler;

int so_init(unsigned int time_quantum, unsigned int io)
{
	int ret;

	if (time_quantum < 1)
		return FAIL;
	if (io < 0 || io > SO_MAX_NUM_EVENTS)
		return FAIL;
	if (tScheduler.tArr != NULL)
		return FAIL;
	tScheduler.io_ev_n = io;
	tScheduler.quantum = time_quantum;
	tScheduler.tArr = calloc(1, sizeof(tStruct));
	DIE(tScheduler.tArr == NULL, "calloc");
	tScheduler.arr_size = 0;
	tScheduler.arr_max_size = 1;
	tScheduler.pq_head = NULL;
	ret = pthread_key_create(&tScheduler.key, NULL);
	DIE(ret != 0, "key create");
	return SUCCESS;
}

/* Checks if current thread should be preempted */
static int isPreempted()
{
	unsigned int *t_index;

	/* get index of current thread in thread array */
	t_index = pthread_getspecific(tScheduler.key);
	if (t_index == NULL)
		return 0;
	if (topPrio(tScheduler.pq_head) != -1) {
		/* check if there is another thread
		 * with higher priority in READY state */
		if (tScheduler.tArr[*t_index].priority
			< topPrio(tScheduler.pq_head))
			return 1;
	}
	/* check if quantum expired */
	if (tScheduler.tArr[*t_index].timer == 0)
		return 1;
	return 0;
}

/* Wakes oldest highest priority thread in READY state */
static void wakeNextThread()
{
	unsigned int t_index;

	t_index = dequeue(&tScheduler.pq_head);
	if (t_index != -1)
		sem_post(tScheduler.tArr[t_index].sem);
}

/* Moves current thread to READY state and executes next one */
static void preemptThread()
{
	unsigned int *running_t_ind;

	running_t_ind = pthread_getspecific(tScheduler.key);
	/* reset timer */
	tScheduler.tArr[*running_t_ind].timer = tScheduler.quantum;
	enqueue(&tScheduler.pq_head,
		*running_t_ind,
		tScheduler.tArr[*running_t_ind].priority);
	wakeNextThread();
	/* block current thread */
	sem_wait(tScheduler.tArr[*running_t_ind].sem);
}

/* After thread is scheduled, execute its function */
static void *start_thread(void *arg)
{
	int ret;
	unsigned int *ind = (unsigned int *) arg;

	/* wait for scheduling, unless its the first thread created */
	if (*ind != 0) {
		ret = sem_wait(tScheduler.tArr[*ind].sem);
	}
	/* set thread specific value to its index in the thread array */
	ret = pthread_setspecific(tScheduler.key, ind);
	DIE(ret != 0, "pthread_setspecific");
	tScheduler.tArr[*ind].func(tScheduler.tArr[*ind].priority);
	/* after finished executing, wake next thread */
	wakeNextThread();
	return  NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority)
{
	int ret;
	unsigned int *t_index;
	tid_t tid = 0;

	if (func == NULL || priority > SO_MAX_PRIO)
		return INVALID_TID;
	/* value for thread specific data, has to be on the heap */
	/* otherwise it will get lost when so_fork finishes */
	t_index = calloc(1, sizeof(int));
	DIE(t_index == NULL, "calloc");
	*t_index = tScheduler.arr_size;
	tScheduler.arr_size += 1;
	/* if array has reached max size, reallocate a bigger one */
	if (tScheduler.arr_size == tScheduler.arr_max_size) {
		tScheduler.tArr = realloc(tScheduler.tArr,
				2 * tScheduler.arr_max_size * sizeof(tStruct));
		DIE(tScheduler.tArr == NULL, "realloc");
		tScheduler.arr_max_size *= 2;
	}

	tScheduler.tArr[*t_index].priority = priority;
	tScheduler.tArr[*t_index].timer = tScheduler.quantum;
	tScheduler.tArr[*t_index].func = func;
	tScheduler.tArr[*t_index].v_addr = t_index;
	tScheduler.tArr[*t_index].io = -1;
	/* semaphore also has to be on heap,
	 * since sem functions use its address,
	 * which might change when realloc is called
	 */
	tScheduler.tArr[*t_index].sem = calloc(1, sizeof(sem_t));
	DIE(tScheduler.tArr[*t_index].sem == NULL, "calloc");
	ret = sem_init(tScheduler.tArr[*t_index].sem, 0, 0);
	DIE(ret == -1, "sem_init");
	/* dont add first created thread to the queue */
	if (*t_index != 0)
		enqueue(&tScheduler.pq_head, *t_index, priority);
	ret = pthread_create(&tid, NULL, start_thread, t_index);
	DIE(ret != 0, "pthread_create");
	tScheduler.tArr[*t_index].tid = tid;

	unsigned int *running_t_ind = pthread_getspecific(tScheduler.key);
	if (running_t_ind != NULL) {
		tScheduler.tArr[*running_t_ind].timer--;
	}

	if (isPreempted())
		preemptThread();

	return tid;
}

void so_end(void)
{
	int i;

	/* wait for all threads to finish and free all memory */
	for (i = 0; i < tScheduler.arr_size; i++) {
		pthread_join(tScheduler.tArr[i].tid, NULL);
		free(tScheduler.tArr[i].v_addr);
		sem_destroy(tScheduler.tArr[i].sem);
		free(tScheduler.tArr[i].sem);
	}
	/* cleanup array, queue and key */
	pthread_key_delete(tScheduler.key);
	if (tScheduler.tArr != NULL)
		free(tScheduler.tArr);
	tScheduler.tArr = NULL;
	if (tScheduler.pq_head != NULL)
		deleteAllNodes(&tScheduler.pq_head);
}

int so_wait(unsigned int io)
{
	unsigned int *running_t_ind;

	if (io >= tScheduler.io_ev_n || io < 0)
		return -1;
	running_t_ind = pthread_getspecific(tScheduler.key);
	DIE(running_t_ind == NULL, "pthread_getspecific");
	/* mark this thread as waiting on io */
	tScheduler.tArr[*running_t_ind].io = io;
	wakeNextThread();
	/* block until signaled */
	sem_wait(tScheduler.tArr[*running_t_ind].sem);
	return 0;
}

int so_signal(unsigned int io)
{
	int woken_up = 0;
	unsigned int *t_index;
	int i;

	if (io >= tScheduler.io_ev_n || io < 0)
		return -1;
	t_index = pthread_getspecific(tScheduler.key);
	DIE(t_index == NULL, "pthread_getspecific");
	for (i = 0; i < tScheduler.arr_size; i++)
		if (tScheduler.tArr[i].io == io && i != *t_index) {
			/* wake up all threads waiting on this io */
			tScheduler.tArr[i].timer = tScheduler.quantum;
			tScheduler.tArr[i].io = -1;
			/* put them in READY state */
			enqueue(&tScheduler.pq_head, i, tScheduler.tArr[i].priority);
			woken_up++;
		}
	tScheduler.tArr[*t_index].timer--;
	if (isPreempted())
		preemptThread();
	return woken_up;
}

void so_exec(void)
{
	unsigned int *t_index;

	t_index = pthread_getspecific(tScheduler.key);
	DIE(t_index == NULL, "pthread_getspecific");
	tScheduler.tArr[*t_index].timer--;
	if (isPreempted())
		preemptThread();
}
