#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>

/**
 *
 */
int manager_handle_box_management(char *register_pipe_name, uint8_t code,
                                  char *session_pipename, char *box_name);

/**
 *
 */
int manager_handle_box_listing(char *register_pipe_name,
                               char *session_pipename);

/**
 *
 */
void manager_shutdown(int signum);

#endif // __MANAGER_H__
