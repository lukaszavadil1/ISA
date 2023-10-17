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
#include <unistd.h>
#include <signal.h>

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

extern int packet_pos;

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

/**
* @brief Create unbouded socket.
*
* @return Socket file descriptor.
*/
int create_socket();

/**
* @brief Handle SIGINT signal.
*
* @param sig Signal number.
*
* @return void
*/
void sigint_handler(int sig);

/**
* @brief Set packet's opcode.
*
* @param opcode Opcode.
* @param packet Pointer to packet.
*
* @return void
*/
void opcode_set(int opcode, char *packet);

/**
* @brief Get packet's opcode.
*
* @param packet Pointer to packet.
*
* @return Opcode.
*/
int opcode_get(char *packet);

/**
* @brief Set packet's file name.
*
* @param file_name File name.
* @param packet Pointer to packet.
*
* @return void
*/
void file_name_set(char *file_name, char *packet);

/**
* @brief Get packet's file name.
*
* @param packet Pointer to packet.
*
* @return File name.
*/
char *file_name_get(char *packet);

/**
* @brief Set packet's mode.
*
* @param mode Mode.
* @param packet Pointer to packet.
*
* @return void
*/
void mode_set(int mode, char *packet);

/**
* @brief Get packet's mode.
*
* @param packet Pointer to packet.
*
* @return Mode.
*/
char *mode_get(char *packet);

/**
* @brief Insert empty byte to packet.
*
* @param packet Pointer to packet.
*
* @return void
*/
void empty_byte_insert(char *packet);

/**
* @brief Get opcode string from opcode.
*
* @param opcode Opcode.
*
* @return Opcode string.
*/
char *opcode_to_str(int opcode);

#endif // UTILS_H