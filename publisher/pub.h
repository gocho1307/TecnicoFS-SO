#ifndef __PUBLISHER_H__
#define __PUBLISHER_H__

/**
 * Cycles reading chars until newline is sent, then it sends the message
 * received until then. When EOF is received it shuts the session down.
 *
 * Input:
 *	- session_pipename: name of the publisher pipe
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int publisher_write_messages(char *session_pipename);

#endif // __PUBLISHER_H__
