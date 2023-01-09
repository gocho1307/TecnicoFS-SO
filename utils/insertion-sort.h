#ifndef __INSERTION_SORT__
#define __INSERTION_SORT__

#include "../common/common.h"

/**
 * Finds the correct place to insert a given box in the sorted boxes array based
 * on their name. After this function is called the array remains in
 * lexicographic order of the box names.
 */
void sorted_insert(box *boxes, int right, box new_box);

#endif // __INSERTION_SORT__
