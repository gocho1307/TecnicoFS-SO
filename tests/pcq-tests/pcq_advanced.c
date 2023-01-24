#include "../../producer-consumer/producer-consumer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 10

int main() {
    // Create an int array
    int *numbers = (int *)malloc(QUEUE_CAPACITY * sizeof(int));
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        numbers[i] = i;
    }

    // Create the queue
    pc_queue_t *queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));
    int result = pcq_create(queue, QUEUE_CAPACITY);
    assert(result == 0);
    assert(queue->pcq_capacity == QUEUE_CAPACITY);

    // Do some basic push and pop operations on the queue
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        assert(queue->pcq_head == i);
        assert(queue->pcq_tail == i);
        result = pcq_enqueue(queue, &numbers[i]);
        assert(result == 0);
        int j = *(int *)pcq_dequeue(queue);
        assert(j == i);
    }
    assert(queue->pcq_head == 0);
    assert(queue->pcq_tail == 0);
    assert(queue->pcq_current_size == 0);

    // Destroy the queue
    result = pcq_destroy(queue);
    assert(result == 0);
    free(queue);

    printf("Successful test.\n");

    return 0;
}
