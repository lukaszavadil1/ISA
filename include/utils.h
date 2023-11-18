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

#define MAX_STR_LEN 256
#define DEFAULT_DATA_SIZE 512
#define DEFAULT_REQUEST_SIZE 512
#define DEFAULT_PACKET_SIZE 516

// Default port number for TFTP.
#define DEFAULT_PORT_NUM 69

// Sizes of static parts of packets.
#define OPCODE_SIZE 2
#define BLOCK_NUMBER_SIZE 2
#define ERROR_CODE_SIZE 2

// Opcode values.
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define OACK 6

// Supported modes.
#define NETASCII 0
#define OCTET 1

// Supported options.
#define TIMEOUT 0
#define TSIZE 1
#define BLKSIZE 2
#define BLKSIZE_MIN 8
#define BLKSIZE_MAX 65464
#define BLKSIZE_DEFAULT 512
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
extern bool last;

// Struct for storing options.
typedef struct Option {
    bool flag;
    long int value;
    int order;
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
* @param port Port number.
* @param addr Pointer to socket address.
*
* @return Socket file descriptor.
*/
int init_socket(int port, struct sockaddr_in *addr);

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
* @param opcode Opcode to be set.
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
* @return Packet's opcode.
*/
int opcode_get(char *packet);

/**
* @brief Set packet's file name.
*
* @param file_name File name to be set.
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
* @return Packet's file name.
*/
char *file_name_get(char *packet);

/**
* @brief Set packet's mode.
*
* @param mode Mode to be set.
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
* @return Packet's mode.
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
* @brief Set packet's block number.
*
* @param block_number Block number to be set.
* @param packet Pointer to packet.
*
* @return void
*/
void block_number_set(int block_number, char *packet);

/**
* @brief Get packet's block number.
*
* @param packet Pointer to packet.
*
* @return Packet's block number.
*/
int block_number_get(char *packet);

/**
* @brief Set packet's data.
*
* @param packet Pointer to packet.
* @param file Pointer to file.
*
* @return void
*/
void data_set(char *packet, FILE *file);

/**
* @brief Get packet's data.
*
* @param packet Pointer to packet.
*
* @return Packet's data.
*/
char *data_get(char *packet);

/**
* @brief Set packet's error code.
*
* @param error_code Error code to be set.
* @param packet Pointer to packet.
*
* @return void
*/
void error_code_set(int error_code, char *packet);

/**
* @brief Get packet's error code.
*
* @param packet Pointer to packet.
*
* @return Packet's error code.
*/
int error_code_get(char *packet);

/**
* @brief Set packet's error message.
*
* @param error_msg Error message to be set.
* @param packet Pointer to packet.
*
* @return void
*/
void error_msg_set(char *error_msg, char *packet);

/**
* @brief Get packet's error message.
*
* @param packet Pointer to packet.
*
* @return Packet's error message.
*/
char *error_msg_get(char *packet);

/**
* @brief Set packet option's flag, value and order.
*
* @param type Option type.
* @param value Option value.
* @param order Option order.
*
* @return void
*/
void option_set(int type, long int value, int order);

/**
* @brief Get packet option's type.
*
* @param name Option name.
*
* @return Option type.
*/
int option_get_type(char *name);

/**
* @brief Get packet option's value.
*
* @param type Option type.
*
* @return Option value.
*/
long int option_get_value(int type);

/**
* @brief Get packet option's order.
*
* @param type Option type.
*
* @return Option order.
*/
int option_get_order(int type);

/**
* @brief Get packet option's flag.
*
* @param type Option type.
*
* @return Option flag.
*/
bool option_get_flag(int type);

/**
* @brief Get packet option's string name.
*
* @param type Option type.
*
* @return Option name.
*/
char *option_get_name(int type);

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

/**
* @brief Handle request packet.
*
* @param packet Pointer to packet.
*
* @return void
*/
void handle_request_packet(char *packet);

/**
* @brief Send request packet.
*
* @param socket Socket file descriptor.
* @param dest_addr Destination address.
* @param opcode Opcode.
* @param file_name File name.
*
* @return void
*/
void send_request_packet(int socket, struct sockaddr_in dest_addr, int opcode, char * file_name);

/**
* @brief Handle ack packet.
*
* @param packet Pointer to packet.
* @param expected_block_number Expected block number.
*
* @return void
*/
void handle_ack_packet(char *packet, int expected_block_number);

/**
* @brief Send ack packet.
*
* @param socket Socket file descriptor.
* @param dest_addr Destination address.
* @param block_number Block number.
*
* @return void
*/
void send_ack_packet(int socket, struct sockaddr_in dest_addr, int block_number);

/**
* @brief Handle oack packet.
*
* @param packet Pointer to packet.
*
* @return void
*/
void handle_oack_packet(char *packet);

/**
* @brief Send oack packet.
*
* @param socket Socket file descriptor.
* @param dest_addr Destination address.
*
* @return void
*/
void send_oack_packet(int socket, struct sockaddr_in dest_addr);

/**
* @brief Handle data packet.
*
* @param packet Pointer to packet.
* @param expected_block_number Expected block number.
* @param file Pointer to file.
*
* @return True if last packet, false otherwise.
*/
bool handle_data_packet(char *packet, int expected_block_number, FILE *file);

/**
* @brief Send data packet.
*
* @param socket Socket file descriptor.
* @param dest_addr Destination address.
* @param block_number Block number.
* @param file Pointer to file.
*
* @return True if last packet, false otherwise.
*/
bool send_data_packet(int socket, struct sockaddr_in dest_addr, int block_number, FILE *file);

/**
* @brief Send error packet.
*
* @param socket Socket file descriptor.
* @param dest_addr Destination address.
* @param error_code Error code.
* @param error_msg Error message.
*
* @return void
*/
void send_error_packet(int socket, struct sockaddr_in dest_addr, int error_code, char *error_msg);

/**
* @brief Display received packet in a formatted way.
*
* @param socket Socket file descriptor.
* @param source_addr Source address.
* @param packet Pointer to packet.
*
* @return void
*/
void display_message(int socket, struct sockaddr_in source_addr, char *packet);

/**
* @brief Display options in a formatted way.
*
* @param packet Pointer to packet.
*
* @return void
*/
void display_options(char *packet);

/**
* @brief Construct full path to file and open it correctly.
*
* @param socket Socket file descriptor.
* @param packet Pointer to packet.
* @param dir_path Directory path.
* @param source_addr Source address.
*
* @return Pointer to file.
*/
FILE *open_file(int socket, char *packet, char *dir_path, struct sockaddr_in source_addr);

/**
* @brief Strlen with maximum length.
*
* @param s String.
* @param maxlen Maximum length.
*
* @return String length.
*/
size_t strnlen(const char *s, size_t maxlen);

#endif // UTILS_H