#include "../producer-consumer/producer-consumer.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define QUEUE_CAPACITY 10

int main () {

	// Create an int array
	int* numbers = (int*) malloc(QUEUE_CAPACITY * sizeof(int));
	for (int i = 0; i < QUEUE_CAPACITY; i++) {
		numbers[i] = i;
	}

	int result;

	pc_queue_t* queue = (pc_queue_t*) malloc (sizeof(pc_queue_t));
	result = pcq_create(queue, QUEUE_CAPACITY);
	assert( result == 0 );

	for ( int i = 0; i < QUEUE_CAPACITY; i++ ) {
		pcq_enqueue(queue, &numbers[i]);
	}
	for (int i = 0; i < QUEUE_CAPACITY; i++ ) {
		printf("%d\n", *(int*)pcq_dequeue(queue));
	}

	result = pcq_destroy(queue);
	assert ( result == 0 );

    printf("Successful test.\n");

	return 0;
}
