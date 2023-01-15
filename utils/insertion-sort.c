/*
 *      File: insertion-sort.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Generic insertion sort algorithm used to insert an element
 *                   in a sorted array while keeping that invariant.
 */

#include "insertion-sort.h"
#include "logging.h"
#include <string.h>

void sorted_insert(box_t *boxes, int right, box_t *new_box) {
    if (right < -1) {
        WARN("Wrong index value, not eligible for insertion");
        return;
    }

    // Inserts the element in the sorted array
    for (; right >= 0 && strcmp(boxes[right].name, new_box->name) > 0;
         right--) {
        boxes[right + 1] = boxes[right];
    }
    boxes[right + 1] = *new_box;
}
