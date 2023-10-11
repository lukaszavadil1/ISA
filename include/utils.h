//
// File: utils.h
//
// Author: Lukáš Zavadil
//
// Description: Header file for helper functions.
//

#ifndef UTILS_H
#define UTILS_H

#define MAX_STR_LEN 1024

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

/**
* @brief Parse port number from string.
*
* @param port_str String containing port number.
*
* @return Port number.
*/
int parse_port(char *port_str);

/**
* @brief Display client's usage.
*
* @return void
*/
void display_client_help();

/**
* @brief Display server's usage.
*
* @return void
*/
void display_server_help();

#endif // UTILS_H