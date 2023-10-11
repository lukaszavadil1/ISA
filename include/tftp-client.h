//
// File: tftp-client.h
//
// Author: Lukáš Zavadil
//
// Description: Header file for tfpt client.
//

#ifndef TFTP_CLIENT_H
#define TFTP_CLIENT_H

#include "utils.h"

/**
* @brief Struct for storing client's command line arguments.
*/
typedef struct ClientArgs {
    char *host_name;
    int port;
    char *file_path;
    char *dest_file_path;
} ClientArgs_t;

/**
* @brief Initialize ClientArgs_t struct.
*
* @param client_args Pointer to ClientArgs_t struct.
*
* @return void
*/
void init_args(ClientArgs_t *client_args);

/**
* @brief Deallocate memory allocated for ClientArgs_t struct.
*
* @param client_args Pointer to ClientArgs_t struct.
*
*/
void free_args(ClientArgs_t *client_args);

/**
* @brief Handle client's command line arguments.
*
* @param argc Number of command line arguments.
* @param argv Command line arguments array.
* @param client_args Pointer to ClientArgs_t struct.
*
* @return void
*/
void parse_args(int argc, char *argv[], ClientArgs_t *client_args);

#endif // TFTP_CLIENT_H