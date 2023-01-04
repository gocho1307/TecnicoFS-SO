#ifndef __MBROKER_H__
#define __MBROKER_H__

#include <sys/types.h>

/**
 *
 */
int mbroker_init(char *pipename, size_t max_sessions);

#endif // __MBROKER_H__
