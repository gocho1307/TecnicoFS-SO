#ifndef __PUBLISHER_H__
#define __PUBLISHER_H__

/**
 *
 */
int publisher_write_messages(char *session_pipename);

/**
 *
 */
void publisher_shutdown(int signum);

#endif // __PUBLISHER_H__
