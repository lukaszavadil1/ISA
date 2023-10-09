//
// File: utils.h
//
// Author: Lukáš Zavadil
//
// Description: Header file for helper functions.
//

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

/**
* @brief Prints error message and exits program.
*
* @param message Error message.
*
* @return void
*/
void error_exit(const char *message);

#endif // UTILS_H