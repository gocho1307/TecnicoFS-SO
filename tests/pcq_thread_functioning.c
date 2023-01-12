#include "../producer-consumer/producer-consumer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define QUEUE_CAPACITY 10

void* enqueuer (void* arg) {

	pc_queue_t* queue = (pc_queue_t*) arg;

	int* numbers = (int*) malloc(2*QUEUE_CAPACITY * sizeof(int));
	for (int i = 0; i < 2*QUEUE_CAPACITY; i++) {
		numbers[i] = i;
	}

	for ( int i = 0; i < 2*QUEUE_CAPACITY; i++ ) {
		//sleep(1);
		pcq_enqueue(queue, &numbers[i]);
	}

	return NULL;
}

void* dequeuer (void* arg) {

	pc_queue_t* queue = (pc_queue_t*) arg;

	int* result;
	for ( int i = 0; i < 2*QUEUE_CAPACITY; i++ ) {
		//sleep(1);
		result = (int*)pcq_dequeue(queue);
		printf("%d\n", *result);
	}

	return NULL;
}

int main () {

	int result;

	pc_queue_t* queue = (pc_queue_t*) malloc (sizeof(pc_queue_t));
	result = pcq_create(queue, QUEUE_CAPACITY);
	assert( result == 0 );

	pthread_t enqueuer_thread;
	pthread_create(&enqueuer_thread, NULL, enqueuer, (void*)queue);

	pthread_t dequeuer_thread;
	pthread_create(&dequeuer_thread, NULL, dequeuer, (void*)queue);

	pthread_join(enqueuer_thread, NULL);
	pthread_join(dequeuer_thread, NULL);

	result = pcq_destroy(queue);
	assert ( result == 0 );

    printf("Successful test.\n");

	return 0;
}
