#ifndef __SUBSCRIBER_H__
#define __SUBSCRIBER_H__

/**
 *
 */
int subscriber_read_messages(char *session_pipename);

/**
 *
 */
void subscriber_shutdown(int signum);

#endif // __SUBSCRIBER_H__
