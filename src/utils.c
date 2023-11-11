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
    if (mode == 1) {
        strcpy(packet + packet_pos, "octet");
        packet_pos += strlen("octet");
    }
    else {
        strcpy(packet + packet_pos, "netascii");
        packet_pos += strlen("netascii");
    }
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
    char *data = malloc(DEFAULT_DATA_SIZE);
    if (data == NULL) {
        error_exit("Data malloc failed.");
    }
    // Copy data from packet.
    strcpy(data, packet + packet_pos);
    // Increment pointer position in packet.
    packet_pos += strlen(data);
    return data;
}

void handle_request_packet(char *packet) {
    int opcode;
    char *file_name, *mode;

    opcode = opcode_get(packet);
    if (opcode != RRQ && opcode != WRQ) {
        error_exit("Invalid opcode, server expected RRQ or WRQ.");
    }
    file_name = file_name_get(packet);
    if (strlen(file_name) == 0) {
        error_exit("Invalid file name.");
    }
    mode = mode_get(packet);
    if (strcmp(mode, "octet") != 0 && strcmp(mode, "netascii") != 0) {
        error_exit("Invalid mode.");
    }
    options_load(packet);
    packet_pos = 0;
}

void handle_ack(char *packet, int expected_block_number) {
    int opcode, block_number;

    opcode = opcode_get(packet);
    if (opcode != ACK) {
        error_exit("Invalid opcode, server expected ACK.");
    }
    block_number = block_number_get(packet);
    if (block_number != expected_block_number) {
        error_exit("Invalid ack block number.");
    }
    packet_pos = 0;
}

void handle_data(char *packet, int expected_block_number) {
    int opcode, block_number;

    opcode = opcode_get(packet);
    if (opcode != DATA) {
        error_exit("Invalid opcode, server expected DATA.");
    }
    block_number = block_number_get(packet);
    if (block_number != expected_block_number) {
        error_exit("Invalid data block number.");
    }
}

void send_data_packet(int socket, struct sockaddr_in dest_addr, int block_number, char *data) {
    char packet[MAX_PACKET_SIZE];
    memset(&packet, 0, MAX_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(DATA, packet);
    block_number_set(block_number, packet);
    data_set(data, packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void send_ack_packet(int socket, struct sockaddr_in dest_addr, int block_number) {
    char packet[MAX_PACKET_SIZE];
    memset(&packet, 0, MAX_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(ACK, packet);
    block_number_set(block_number, packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void send_request_packet(int socket, struct sockaddr_in dest_addr, int opcode, char * file_name) {
    char packet[MAX_PACKET_SIZE];
    memset(&packet, 0, MAX_PACKET_SIZE);

    opcode_set(opcode, packet);
    file_name_set(file_name, packet);
    empty_byte_insert(packet);
    mode_set(1, packet);
    empty_byte_insert(packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void display_message(int socket, struct sockaddr_in source_addr, char *packet) {
    struct sockaddr_in dest_addr;
    socklen_t dest_addr_size = sizeof(dest_addr);
    memset(&dest_addr, 0, dest_addr_size);

    if (getsockname(socket, (struct sockaddr *)&dest_addr, &dest_addr_size) < 0) {
        error_exit("Getsockname failed.");
    }
    int dest_port = ntohs(dest_addr.sin_port);
    int src_port = ntohs(source_addr.sin_port);
    char *src_ip = inet_ntoa(source_addr.sin_addr);
    int opcode = opcode_get(packet);
    int block_number;
    char *mode;
    char *file_name;
    int error_code;
    char *error_msg;
    switch (opcode) {
        case RRQ:
            file_name = file_name_get(packet);
            mode = mode_get(packet);
            fprintf(stderr, "RRQ: %s:%d \"%s\" %s\n", src_ip, src_port, file_name, mode);
            break;
        case WRQ:
            file_name = file_name_get(packet);
            mode = mode_get(packet);
            fprintf(stderr, "WRQ: %s:%d \"%s\" %s\n", src_ip, src_port, file_name, mode);
            break;
        case DATA:
            block_number = block_number_get(packet);
            fprintf(stderr, "DATA: %s:%d:%d %d\n", src_ip, src_port, dest_port, block_number);
            break;
        case ACK:
            block_number = block_number_get(packet);
            fprintf(stderr, "ACK: %s:%d %d\n", src_ip, src_port, block_number);
            break;
        case ERROR:
            error_code = error_code_get(packet);
            error_msg = error_msg_get(packet);
            fprintf(stderr, "ERROR: %s:%d:%d %d \"%s\"\n", src_ip, src_port, dest_port, error_code, error_msg);
            break;
        case OACK:
            // TODO - add options
            fprintf(stderr, "OACK: %s:%d\n", src_ip, src_port);
            break;
        default:
            error_exit("Invalid opcode.");
    }
    packet_pos = 0;
    memset(&packet, 0, MAX_PACKET_SIZE);
}

void error_code_set(int error_code, char *packet) {
    // Save error code in network byte order.
    error_code = htons(error_code);
    // Copy error code to packet.
    memcpy(packet + packet_pos, &(error_code), 2);
    // Increment pointer position in packet.
    packet_pos += 2;
}

int error_code_get(char *packet) {
    int error_code;
    // Copy error code from packet.
    memcpy(&error_code, packet + packet_pos, 2);
    // Convert error code to host byte order.
    error_code = ntohs(error_code);
    // Increment pointer position in packet.
    packet_pos += 2;
    return error_code;
}

void error_msg_set(char *error_message, char *packet) {
    // Copy error message to packet.
    strcpy(packet + packet_pos, error_message);
    // Increment pointer position in packet.
    packet_pos += strlen(error_message);
}

char *error_msg_get(char *packet) {
    char *error_message = malloc(MAX_STR_LEN);
    if (error_message == NULL) {
        error_exit("Error message malloc failed.");
    }
    // Copy error message from packet.
    strcpy(error_message, packet + packet_pos);
    // Increment pointer position in packet.
    packet_pos += strlen(error_message);
    return error_message;
}

void send_error_packet(int socket, struct sockaddr_in dest_addr, int error_code, char *error_message) {
    char packet[MAX_PACKET_SIZE];
    memset(&packet, 0, MAX_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(ERROR, packet);
    error_code_set(error_code, packet);
    error_msg_set(error_message, packet);
    empty_byte_insert(packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
}