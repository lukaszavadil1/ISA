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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Maximal length of string.
#define MAX_STR_LEN 512
#define MAX_PACKET_SIZE 1024

// Opcode values based on TFTP RFC 1350.
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define OACK 6

// Default port number
#define DEFAULT_PORT_NUM 69

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