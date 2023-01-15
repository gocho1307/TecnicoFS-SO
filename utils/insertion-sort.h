#ifndef __INSERTION_SORT__
#define __INSERTION_SORT__

#include "../mbroker/mbroker.h"

/**
 * Finds the correct place to insert a given box in the sorted boxes array based
 * on their name. After this function is called the array remains in
 * lexicographic order of the box names.
 */
void sorted_insert(box_t *boxes, int right, box_t *new_box);

#endif // __INSERTION_SORT__
