//
// File: tftp-server.h
//
// Author: Lukáš Zavadil (xzavad20)
//
// Description: Header file for tftp server.
//

#ifndef TFTP_SERVER_H
#define TFTP_SERVER_H

#include "utils.h"

/**
* @brief Struct for storing server's command line arguments.
*/
typedef struct ServerArgs {
    int port;
    char *dir_path;
} ServerArgs_t;

FILE *file;

/**
* @brief Initialize ServerArgs_t struct.
*
* @param server_args Pointer to ServerArgs_t struct.
*
* @return void
*/
void init_args(ServerArgs_t *server_args);

/**
* @brief Deallocate memory allocated for ServerArgs_t struct.
*
* @param server_args Pointer to ServerArgs_t struct.
*
* @return void
*/
void free_args(ServerArgs_t *server_args);

/**
* @brief Handle server's command line arguments.
*
* @param argc Number of command line arguments.
* @param argv Command line arguments array.
* @param server_args Pointer to ServerArgs_t struct.
*
* @return void
*/
void parse_args(int argc, char *argv[], ServerArgs_t *server_args);

#endif // TFTP_SERVER_H