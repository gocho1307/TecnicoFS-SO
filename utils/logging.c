/*
 *      File: logging.c
 *      Authors: Gonçalo Sampaio Bárias (ist1103124)
 *               Pedro Perez Vieira (ist1100064)
 *      Description: Basic logging management file.
 */

#include "logging.h"

log_level_t g_level = LOG_QUIET;

void set_log_level(log_level_t level) { g_level = level; }
