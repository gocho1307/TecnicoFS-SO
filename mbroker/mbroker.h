#ifndef __MBROKER_H__
#define __MBROKER_H__

#include <sys/types.h>

/**
 *
 */
int mbroker_init(char *pipename, size_t max_sessions);

/**
 *
 */
int workers_init(int num_threads);

/**
 *
 */
void mbroker_shutdown(int signum);

#endif // __MBROKER_H__
