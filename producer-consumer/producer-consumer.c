/*
 *      File: producer-consumer.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Program that implements a priority queue used by the
 *                   mbroker.
 */

#include "producer-consumer.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {
    (void)queue;
    (void)capacity;
    return 0;
}

int pcq_destroy(pc_queue_t *queue) {
    (void)queue;
    return 0;
}

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    (void)queue;
    (void)elem;
    return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
    (void)queue;
    return 0;
}
