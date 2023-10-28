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
#include <dirent.h>

// Maximal length of string.
#define MAX_STR_LEN 256
#define DEFAULT_DATA_SIZE 512
#define MAX_PACKET_SIZE 516
#define DEFAULT_PORT_NUM 69

// Opcode values based on TFTP RFC 1350.
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define OACK 6

#define TIMEOUT 0
#define TSIZE 1
#define BLKSIZE 2
#define TIMEOUT_NAME "timeout"
#define TSIZE_NAME "tsize"
#define BLKSIZE_NAME "blksize"
#define NUM_OPTIONS 3

// Error codes.
#define ERR_NOT_DEFINED 0
#define ERR_FILE_NOT_FOUND 1
#define ERR_ACCESS_VIOLATION 2
#define ERR_DISK_FULL 3
#define ERR_ILLEGAL_OPERATION 4
#define ERR_UNKNOWN_TRANSFER_ID 5
#define ERR_FILE_ALREADY_EXISTS 6
#define ERR_NO_SUCH_USER 7

extern int packet_pos;

// Struct for storing options.
typedef struct Option {
    bool flag;
    long int value;
} Option_t;

// Array of options.
extern Option_t options[NUM_OPTIONS];

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

void option_set(int type, long int value);

bool option_get_flag(int type);

long int option_get_value(int type);

char *option_get_name(int type);

int option_get_type(char *name);

/**
* @brief Load options from packet.
*
* @param packet Pointer to packet.
*
* @return void
*/
void options_load(char *packet);

/**
* @brief Set options in packet.
*
* @param packet Pointer to packet.
*
* @return void
*/
void options_set(char *packet);

void block_number_set(int block_number, char *packet);

int block_number_get(char *packet);

void data_set(char *data, char *packet);

char *data_get(char *packet);

void error_code_set(int error_code, char *packet);

int error_code_get(char *packet);

void error_msg_set(char *error_msg, char *packet);

char *error_msg_get(char *packet);

void send_error_packet(int socket, struct sockaddr_in dest_addr, int error_code, char *error_msg);

void handle_client_request(char *packet);

void handle_ack(char *packet, int expected_block_number);

void handle_data(char *packet, int expected_block_number);

void print_oack_packet();

void print_ack_packet(int block_number);

void print_data_packet(int block_number, char *data);

void print_error_packet(int error_code, char *error_msg);

void display_message(int socket, struct sockaddr_in source_addr, char *packet);

#endif // UTILS_H