#ifndef __SUBSCRIBER_H__
#define __SUBSCRIBER_H__

/**
 * Stays in loop waiting for messages, checks if they have correct
 * code number and then prints it to stdout.
 *
 * Input:
 *	- session_pipename: Name of the subscriber pipe
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int subscriber_read_messages(char *session_pipename);

/**
 * SIGINT signal handler.
 */
void subscriber_shutdown(int signum);

#endif // __SUBSCRIBER_H__
