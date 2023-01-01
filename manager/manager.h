#ifndef __MANAGER_H__
#define __MANAGER_H__

/**
 *
 */
int handle_box_create(char *register_pipe_name, char *box_name);

/**
 *
 */
int handle_box_remove(char *register_pipe_name, char *box_name);

/**
 *
 */
int handle_box_management(char *box_name);

/**
 *
 */
int handle_box_listing(char *register_pipe_name);

#endif // __MANAGER_H__
