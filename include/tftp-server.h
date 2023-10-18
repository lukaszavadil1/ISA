//
// File: tftp-server.h
//
// Author: Lukáš Zavadil
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

void handle_client_request(char *packet, int *opcode, char *file_name, char *mode);

void print_client_request(int opcode, char *file_name, char *mode);

#endif // TFTP_SERVER_H