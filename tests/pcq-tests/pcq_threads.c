#include "../../producer-consumer/producer-consumer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QUEUE_CAPACITY 1000
#define THREAD_COUNT 100
#define VALUE_TO_WRITE 10

void *enqueuer(void *arg);
void *dequeuer(void *arg);

/**
 *
 */
int main() {
    pc_queue_t *queue = (pc_queue_t *)malloc(sizeof(pc_queue_t));
    assert(pcq_create(queue, QUEUE_CAPACITY) == 0);
    assert(queue->pcq_capacity == QUEUE_CAPACITY);

    pthread_t enqueuer_threads[THREAD_COUNT];
    pthread_t dequeuer_threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_create(&enqueuer_threads[i], NULL, enqueuer,
                              (void *)queue) == 0);
        assert(pthread_create(&dequeuer_threads[i], NULL, dequeuer,
                              (void *)queue) == 0);
    }
    for (int i = 0; i < THREAD_COUNT; i++) {
        assert(pthread_join(enqueuer_threads[i], NULL) == 0);
        assert(pthread_join(dequeuer_threads[i], NULL) == 0);
    }

    assert(pcq_destroy(queue) == 0);
    free(queue);

    printf("Successful test.\n");

    return 0;
}

void *enqueuer(void *arg) {
    pc_queue_t *queue = (pc_queue_t *)arg;

    int numbers[VALUE_TO_WRITE];
    for (int i = 0; i < VALUE_TO_WRITE; i++) {
        // sleep(1);
        numbers[i] = i;
        assert(pcq_enqueue(queue, &numbers[i]) == 0);
    }

    return NULL;
}

void *dequeuer(void *arg) {
    pc_queue_t *queue = (pc_queue_t *)arg;

    for (int i = 0; i < VALUE_TO_WRITE; i++) {
        // sleep(1);
        assert(pcq_dequeue(queue) != NULL);
    }

    return NULL;
}
