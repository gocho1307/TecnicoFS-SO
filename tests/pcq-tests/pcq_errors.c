#include "../../producer-consumer/producer-consumer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define QUEUE_CAPACITY 10

/**
 *
 */
int main() {
    int element = 1;

    assert(pcq_create(NULL, QUEUE_CAPACITY) == -1);
    pc_queue_t *queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));
    assert(pcq_create(queue, 0) == -1);
    assert(pcq_create(queue, QUEUE_CAPACITY) == 0);

    assert(pcq_destroy(NULL) == -1);
    assert(pcq_destroy(queue) == 0);
    assert(pcq_destroy(queue) == -1);

    assert(pcq_enqueue(queue, &element) == -1);
    assert(pcq_enqueue(NULL, &element) == -1);

    assert(pcq_dequeue(queue) == NULL);
    assert(pcq_dequeue(NULL) == NULL);

    assert(pcq_create(queue, QUEUE_CAPACITY) == 0);

    assert(pcq_enqueue(queue, &element) == 0);

    assert(*(int *)pcq_dequeue(queue) == element);

    pc_queue_t *tmp_queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));

    assert(pcq_destroy(tmp_queue) == -1);

    assert(pcq_enqueue(tmp_queue, &element) == -1);

    assert(pcq_dequeue(tmp_queue) == NULL);

    free(queue);
    free(tmp_queue);

    printf("Successful test.\n");

    return 0;
}
