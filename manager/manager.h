#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>

#define MANAGER_CREATE ('c')
#define MANAGER_REMOVE ('r')
#define MANAGER_LISTING ('l')

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

#endif // __MANAGER_H__
