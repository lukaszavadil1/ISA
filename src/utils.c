//
// File: utils.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of helper functions.
//

#include "../include/utils.h"

int packet_pos = 0;

// Set default values for options.
Option_t options[NUM_OPTIONS] = {
    [TIMEOUT] = {
        .flag = false,
        .value = 1
    },
    [TSIZE] = {
        .flag = false,
        .value = 0
    },
    [BLKSIZE] = {
        .flag = false,
        .value = 512
    }
};

void error_exit(const char *message) {
    (errno == 0) ? fprintf(stderr, "Error: %s\n", message) : fprintf(stderr, "Error: %s (%s)\n", message, strerror(errno));
    exit(EXIT_FAILURE);
}

int parse_port(char *port_str) {
    char *endptr = malloc(MAX_STR_LEN);
    if (endptr == NULL) {
        error_exit("Port string malloc failed.");
    }
    int port = (int)strtol(port_str, &endptr, 10);
    if (*endptr != '\0' || port > 65535 || port < 1) {
        error_exit("Invalid port number.");
    }
    return port;
}

void display_client_help() {
    printf("Usage: bin/tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath\n");
    printf("Options:\n");
    printf("  -h  IP address or host name of the TFTP server.\n");
    printf("  -p  Port number of the TFTP server.\n");
    printf("  -f  Path to the file on the TFTP server.\n");
    printf("  -t  Path to the destination file.\n");
}

void display_server_help() {
    printf("Usage: bin/tftp-server [-p port] root_dirpath\n");
    printf("Options:\n");
    printf("  -p  Port number of the TFTP server.\n");
    printf("  -d  Path to the directory with files.\n");
}

int create_socket() {
    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error_exit("Failed to create socket.");
    }
    return sock_fd;
}

void sigint_handler(int sig) {
    printf("\nSIGINT (signal %d) received. Exiting...\n", sig);
    exit(EXIT_SUCCESS);
}

void opcode_set(int opcode, char *packet) {
    // Save opcode in network byte order.
    opcode = htons(opcode);
    // Copy opcode to first two bytes of packet.
    memcpy(packet, &(opcode), 2);
    // Increment pointer position in packet.
    packet_pos += 2;
}

int opcode_get(char *packet) {
    int opcode;
    // Copy opcode from first two bytes of packet.
    memcpy(&opcode, packet, 2);
    // Convert opcode to host byte order.
    opcode = ntohs(opcode);
    // Increment pointer position in packet.
    packet_pos += 2;
    return opcode;
}

void file_name_set(char *file_name, char *packet) {
    // Copy file name to packet.
    strcpy(packet + packet_pos, file_name);
    // Increment pointer position in packet.
    packet_pos += strlen(file_name);
}

char *file_name_get(char *packet) {
    char *file_name = malloc(MAX_STR_LEN);
    if (file_name == NULL) {
        error_exit("File name malloc failed.");
    }
    // Copy file name from packet.
    strcpy(file_name, packet + packet_pos);
    // Increment pointer position in packet.
    packet_pos += strlen(file_name) + 1;
    return file_name;
}

void mode_set(int mode, char *packet) {
    // Copy mode to packet.
    strcpy(packet + packet_pos, mode == 1 ? "octet" : "netascii");
    // Increment pointer position in packet.
    packet_pos += strlen(mode == 1 ? "octet" : "netascii");
}

char *mode_get(char *packet) {
    char *mode = malloc(MAX_STR_LEN);
    if (mode == NULL) {
        error_exit("Mode malloc failed.");
    }
    // Copy mode from packet.
    strcpy(mode, packet + packet_pos);
    // Increment pointer position in packet.
    packet_pos += strlen(mode) + 1;
    return mode;
}

void empty_byte_insert(char *packet) {
    memcpy(packet + packet_pos, "\0", 1);
    packet_pos++;
}

char *opcode_to_str(int opcode) {
    switch (opcode) {
        case RRQ:
            return "RRQ";
        case WRQ:
            return "WRQ";
        case DATA:
            return "DATA";
        case ACK:
            return "ACK";
        case ERROR:
            return "ERROR";
        case OACK:
            return "OACK";
        default:
            return "UNKNOWN";
    }
}

// Define functions for setting and getting options.
void option_set(int type, long int value) {
    options[type].flag = true;
    options[type].value = value;
}

bool option_get_flag(int type) {
    return options[type].flag;
}

long int option_get_value(int type) {
    return options[type].value;
}

// Define functions for setting and getting option names and types.
char *option_get_name(int type) {
    switch (type) {
        case TIMEOUT:
            return TIMEOUT_NAME;
        case TSIZE:
            return TSIZE_NAME;
        case BLKSIZE:
            return BLKSIZE_NAME;
        default:
            return NULL;
    }
}

int option_get_type(char *name) {
    if (strcmp(name, TIMEOUT_NAME) == 0) {
        return TIMEOUT;
    }
    else if (strcmp(name, TSIZE_NAME) == 0) {
        return TSIZE;
    }
    else if (strcmp(name, BLKSIZE_NAME) == 0) {
        return BLKSIZE;
    }
    else {
        return -1;
    }
}

// Define functions for setting and getting options in packets.
void options_set(char *packet) {
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (option_get_flag(i) == true) {
            // Copy option name to packet.
            strcpy(packet + packet_pos, option_get_name(i));
            // Increment pointer position in packet.
            packet_pos += strlen(option_get_name(i));
            // Copy option value to packet.
            sprintf(packet + packet_pos, "%ld", option_get_value(i));
            // Increment pointer position in packet.
            packet_pos += strlen(packet + packet_pos) + 1;
        }
    }
}

void options_load(char *packet) {
    char *endptr, *name;
    int type;
    long int value;
    while (packet[packet_pos] != '\0') {
        name = packet + packet_pos;
        type = option_get_type(name);
        if (type == -1) {
            error_exit("Invalid option type.");
        }
        if (option_get_flag(type) == true) {
            error_exit("Duplicate option.");
        }
        packet_pos += strlen(name) + 1;
        value = strtol(packet + packet_pos, &endptr, 10);
        if (*endptr != '\0') {
            error_exit("Invalid option value.");
        }
        option_set(type, value);
        packet_pos += strlen(packet + packet_pos) + 1;
    }
}

void block_number_set(int block_number, char *packet) {
    // Save block number in network byte order.
    block_number = htons(block_number);
    // Copy block number to packet.
    memcpy(packet + packet_pos, &(block_number), 2);
    // Increment pointer position in packet.
    packet_pos += 2;
}

int block_number_get(char *packet) {
    int block_number;
    // Copy block number from packet.
    memcpy(&block_number, packet + packet_pos, 2);
    // Convert block number to host byte order.
    block_number = ntohs(block_number);
    // Increment pointer position in packet.
    packet_pos += 2;
    return block_number;
}

void data_set(char *data, char *packet) {
    // Copy data to packet.
    strcpy(packet + packet_pos, data);
    // Increment pointer position in packet.
    packet_pos += strlen(data);
}

char *data_get(char *packet) {
    char *data = malloc(MAX_STR_LEN);
    if (data == NULL) {
        error_exit("Data malloc failed.");
    }
    // Copy data from packet.
    strcpy(data, packet + packet_pos);
    // Increment pointer position in packet.
    packet_pos += strlen(data);
    return data;
}

void handle_client_request(char *packet, int *opcode, char *file_name, char *mode) {
    *opcode = opcode_get(packet);
    if (*opcode != RRQ && *opcode != WRQ) {
        error_exit("Invalid opcode, server expected RRQ or WRQ.");
    }
    strcpy(file_name, file_name_get(packet));
    strcpy(mode, mode_get(packet));
    for (int i = 0; i < MAX_PACKET_SIZE; i++) {
        printf("%c ", packet[i]);
    }
    options_load(packet);
}

void print_client_request(int opcode, char *file_name, char *mode) {
    char *opcode_str = opcode_to_str(opcode);
    if (opcode_str == NULL) {
        error_exit("Opcode to string conversion failed.");
    }
    printf("#########################################\n");
    printf("Client request info:\n\n");
    printf("Opcode: %s\n", opcode_str);
    printf("File name: %s\n", file_name);
    printf("Mode: %s\n", mode);
    if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
        printf("Options:\n");
    }
    if (options[TIMEOUT].flag) {
        printf("Timeout: %ld\n", options[TIMEOUT].value);
    }
    if (options[TSIZE].flag) {
        printf("Tsize: %ld\n", options[TSIZE].value);
    }
    if (options[BLKSIZE].flag) {
        printf("Blksize: %ld\n", options[BLKSIZE].value);
    }
    printf("#########################################\n\n");
}

void print_oack_packet() {
    printf("#########################################\n");
    printf("Packet info\n\n");
    printf("Opcode: %s\n", opcode_to_str(OACK));
    printf("Options:\n");
    if (options[TIMEOUT].flag) {
        printf("Timeout: %ld\n", options[TIMEOUT].value);
    }
    if (options[TSIZE].flag) {
        printf("Tsize: %ld\n", options[TSIZE].value);
    }
    if (options[BLKSIZE].flag) {
        printf("Blksize: %ld\n", options[BLKSIZE].value);
    }
    printf("#########################################\n\n");
}

void print_ack_packet(int block_number) {
    printf("#########################################\n");
    printf("Packet info\n\n");
    printf("Opcode: %s\n", opcode_to_str(ACK));
    printf("Block number: %d\n", block_number);
    printf("#########################################\n\n");
}

void print_data_packet(int block_number, char *data) {
    printf("#########################################\n");
    printf("Packet info\n\n");
    printf("Opcode: %s\n", opcode_to_str(DATA));
    printf("Block number: %d\n", block_number);
    printf("Data: %s\n", data);
    printf("#########################################\n\n");
}