#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>

#define MANAGER_CREATE ('c')
#define MANAGER_REMOVE ('r')
#define MANAGER_LISTING ('l')

/**
 * Sends the create and remove requests to the register FIFO, waits for the
 * response and prints out result to stdout.
 *
 * Input:
 *	- code: Request code number
 *	- session_pipename: Name of the pipe
 *	- box_name: Contains the name of the desired box.
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int manager_handle_box_management(char *register_pipe_name, uint8_t code,
                                  char *session_pipename, char *box_name);

/**
 * Sends the listing request to the register FIFO, waits for the
 * response and prints out result to stdout.
 *
 * Input:
 *	- code: Request code number
 *	- session_pipename: Name of the pipe
 *
 *	Returns 0 if successful, -1 otherwise.
 */
int manager_handle_box_listing(char *register_pipe_name,
                               char *session_pipename);

#endif // __MANAGER_H__
