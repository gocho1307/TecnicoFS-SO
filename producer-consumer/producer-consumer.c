/*
 *      File: producer-consumer.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that implements a priority queue used by the
 *                   mbroker.
 */

#include "producer-consumer.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {

	queue->pcq_capacity = capacity;
	queue->pcq_buffer = (void **) malloc(capacity * sizeof(void *));

	mutex_init(&(queue->pcq_current_size_lock));
	mutex_lock(&(queue->pcq_current_size_lock));
	queue->pcq_current_size = 0;
	mutex_unlock(&(queue->pcq_current_size_lock));

	mutex_init(&(queue->pcq_head_lock));
	mutex_lock(&(queue->pcq_head_lock));
	queue->pcq_head = 0;
	mutex_unlock(&(queue->pcq_head_lock));

	mutex_init(&(queue->pcq_tail_lock));
	mutex_lock(&(queue->pcq_tail_lock));
	queue->pcq_tail = 0;
	mutex_unlock(&(queue->pcq_tail_lock));

	mutex_init(&(queue->pcq_pusher_condvar_lock));
	pthread_cond_init(&(queue->pcq_pusher_condvar), NULL);

	mutex_init(&(queue->pcq_popper_condvar_lock));
	pthread_cond_init(&(queue->pcq_popper_condvar), NULL);

    return 0;
}

int pcq_destroy(pc_queue_t *queue) {

	if (queue->pcq_buffer != NULL) {
		free(queue->pcq_buffer);
	}

	mutex_destroy(&(queue->pcq_current_size_lock));
	mutex_destroy(&(queue->pcq_head_lock));
	mutex_destroy(&(queue->pcq_tail_lock));
	mutex_destroy(&(queue->pcq_pusher_condvar_lock));
	cond_destroy(&(queue->pcq_pusher_condvar));
	mutex_destroy(&(queue->pcq_popper_condvar_lock));
	cond_destroy(&(queue->pcq_popper_condvar));

    return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {

	int isfull = 0;
	mutex_lock(&(queue->pcq_current_size_lock));
	if ( queue->pcq_current_size == queue->pcq_capacity ) {
		isfull = 1;
	}
	mutex_unlock(&(queue->pcq_current_size_lock));

	if (isfull) {
		mutex_lock(&(queue->pcq_pusher_condvar_lock));
		cond_wait(&(queue->pcq_pusher_condvar), &(queue->pcq_pusher_condvar_lock));
		mutex_unlock(&(queue->pcq_pusher_condvar_lock));
	}

	mutex_lock(&(queue->pcq_current_size_lock));
	mutex_lock(&(queue->pcq_tail_lock));
	queue->pcq_buffer[++queue->pcq_tail] = elem;
	queue->pcq_current_size++;
	mutex_unlock(&(queue->pcq_tail_lock));
	mutex_unlock(&(queue->pcq_current_size_lock));

	return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {

	int isempty = 0;
	mutex_lock(&(queue->pcq_current_size_lock));
	if ( queue->pcq_current_size == 0 ) {
		isempty = 1;
	}

	if (isempty) {
		mutex_lock(&(queue->pcq_popper_condvar_lock));
		cond_wait(&(queue->pcq_popper_condvar), &(queue->pcq_popper_condvar_lock));
		mutex_unlock(&(queue->pcq_popper_condvar_lock));
	}

	void* elem = NULL;
	mutex_lock(&(queue->pcq_current_size_lock));
	mutex_lock(&(queue->pcq_tail_lock));
	elem = queue->pcq_buffer[queue->pcq_tail--];
	queue->pcq_current_size--;
	mutex_unlock(&(queue->pcq_tail_lock));
	mutex_unlock(&(queue->pcq_current_size_lock));

    return elem;
}
