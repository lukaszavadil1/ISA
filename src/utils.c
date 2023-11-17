//
// File: utils.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of helper functions.
//

#include "../include/utils.h"

int packet_pos = 0;
bool last = false;

// Set default values for options.
Option_t options[NUM_OPTIONS];

void error_exit(const char *message) {
    (errno == 0) ? fprintf(stderr, "Error: %s\n", message) : fprintf(stderr, "Error: %s (%s)\n", message, strerror(errno));
    exit(EXIT_FAILURE);
}

int parse_port(char *port_str) {
    char *endptr = malloc(MAX_STR_LEN);
    if (endptr == NULL) {
        error_exit("Port string malloc failed.");
    }
    // Convert port string to int.
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

int init_socket(int port, struct sockaddr_in *server_addr) {
    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        error_exit("Failed to create socket.");
    }
    // Set Ipv4 address family and port.
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    return sock_fd;
}

void sigint_handler(int sig) {
    printf("\nSIGINT (signal %d) received. Exiting...\n", sig);
    exit(EXIT_SUCCESS);
}

void opcode_set(int opcode, char *packet) {
    // Save opcode inside packet in network byte order.
    *(int *)packet = htons(opcode);
    packet_pos += OPCODE_SIZE;
}

int opcode_get(char *packet) {
    int opcode;
    // Get opcode from packet.
    memcpy(&opcode, packet, OPCODE_SIZE);
    // Convert opcode to host byte order.
    opcode = ntohs(opcode);
    packet_pos += OPCODE_SIZE;
    return opcode;
}

void file_name_set(char *file_name, char *packet) {
    // Save file name inside packet.
    strcpy(packet_pos + packet, file_name);
    packet_pos += strlen(file_name);
}

char *file_name_get(char *packet) {
    char *file_name = malloc(MAX_STR_LEN);
    if (file_name == NULL) {
        error_exit("File name malloc failed.");
    }
    // Get file name from packet.
    strcpy(file_name, packet_pos + packet);
    packet_pos += strlen(file_name) + 1;
    return file_name;
}

void mode_set(int mode, char *packet) {
    // Save mode inside packet.
    if (mode == OCTET) {
        strcpy(packet_pos + packet, "octet");
        packet_pos += strlen("octet");
    }
    else if (mode == NETASCII) {
        strcpy(packet_pos + packet, "netascii");
        packet_pos += strlen("netascii");
    }
    else {
        error_exit("Mode not supported.");
    }
}

char *mode_get(char *packet) {
    char *mode = malloc(MAX_STR_LEN);
    if (mode == NULL) {
        error_exit("Mode malloc failed.");
    }
    // Get mode from packet.
    strcpy(mode, packet_pos + packet);
    packet_pos += strlen(mode) + 1;
    return mode;
}

void empty_byte_insert(char *packet) {
    packet[packet_pos++] = '\0';
}

void block_number_set(int block_number, char *packet) {
    // Save block number inside packet in network byte order.
    *(int *)(packet + packet_pos) = htons(block_number);
    packet_pos += BLOCK_NUMBER_SIZE;
}

int block_number_get(char *packet) {
    int block_number;
    // Copy block number from packet.
    memcpy(&block_number, packet + packet_pos, BLOCK_NUMBER_SIZE);
    // Convert block number to host byte order.
    block_number = ntohs(block_number);
    packet_pos += BLOCK_NUMBER_SIZE;
    return block_number;
}

void data_set(char *data, char *packet) {
    // Save data inside packet.
    strcpy(packet + packet_pos, data);
    packet_pos += strlen(data);
}

char *data_get(char *packet) {
    char *data = malloc(DEFAULT_DATA_SIZE);
    if (data == NULL) {
        error_exit("Data malloc failed.");
    }
    // Get data from packet.
    strcpy(data, packet + packet_pos);
    packet_pos += strlen(data);
    return data;
}

void handle_request_packet(char *packet) {
    int opcode;
    char *file_name = malloc(MAX_STR_LEN);
    char *mode = malloc(MAX_STR_LEN);
    if (file_name == NULL || mode == NULL) {
        error_exit("File name or mode malloc failed.");
    }

    opcode = opcode_get(packet);
    if (opcode != RRQ && opcode != WRQ) {
        error_exit("Invalid opcode, server expected RRQ or WRQ.");
    }
    file_name = file_name_get(packet);
    if (strlen(file_name) == 0 || strlen(file_name) > MAX_STR_LEN) {
        error_exit("Invalid file name.");
    }
    mode = mode_get(packet);
    if (strcmp(mode, "octet") != 0 && strcmp(mode, "netascii") != 0) {
        error_exit("Unsupported mode.");
    }
    options_load(packet);
    free(file_name);
    free(mode);
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

bool handle_data(char *packet, int expected_block_number, FILE *file) {
    int opcode, block_number;
    char data[DEFAULT_DATA_SIZE];

    opcode = opcode_get(packet);
    if (opcode != DATA) {
        error_exit("Invalid opcode, server expected DATA.");
    }
    block_number = block_number_get(packet);
    if (block_number != expected_block_number) {
        error_exit("Invalid data block number.");
    }
    strcpy(data, data_get(packet));
    for (size_t i = 0; i < strlen(data); i++) {
        fputc(data[i], file);
    }
    if (strlen(data) + 1 < DEFAULT_DATA_SIZE) {
        return true;
    }
    return false;
}

bool send_data_packet(int socket, struct sockaddr_in dest_addr, int block_number, FILE *file) {
    char packet[DEFAULT_PACKET_SIZE];
    int c;
    memset(packet, 0, DEFAULT_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(DATA, packet);
    block_number_set(block_number, packet);
    while ((c = fgetc(file)) != EOF) {
        //printf("Packet pos: %d\n", packet_pos);
        packet[packet_pos++] = (char)c;
        //printf("Packet char: %c\n", (char) c);
        if (packet_pos == DEFAULT_PACKET_SIZE - 1) {
            break;
        }
    }
    packet[packet_pos] = '\0';
    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    if (packet_pos < DEFAULT_PACKET_SIZE - 1) {
        packet_pos = 0;
        return true;
    }
    packet_pos = 0;
    return false;
}

void send_ack_packet(int socket, struct sockaddr_in dest_addr, int block_number) {
    char packet[DEFAULT_PACKET_SIZE];
    memset(packet, 0, DEFAULT_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(ACK, packet);
    block_number_set(block_number, packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void send_oack_packet(int socket, struct sockaddr_in dest_addr) {
    char packet[DEFAULT_PACKET_SIZE];
    memset(packet, 0, DEFAULT_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(OACK, packet);
    options_set(packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void send_request_packet(int socket, struct sockaddr_in dest_addr, int opcode, char *file_name) {
    char packet[DEFAULT_PACKET_SIZE];
    memset(packet, 0, DEFAULT_PACKET_SIZE);

    opcode_set(opcode, packet);
    file_name_set(file_name, packet);
    empty_byte_insert(packet);
    mode_set(OCTET, packet);
    empty_byte_insert(packet);
    option_set(BLKSIZE, 512, 1);
    option_set(TIMEOUT, 1, 0);
    options_set(packet);
    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void display_message(int socket, struct sockaddr_in source_addr, char *packet) {
    struct sockaddr_in dest_addr;
    socklen_t dest_addr_size = sizeof(dest_addr);
    memset(&dest_addr, 0, dest_addr_size);

    // Get required information for printing message.
    if (getsockname(socket, (struct sockaddr *)&dest_addr, (socklen_t *)&dest_addr_size) < 0) {
        error_exit("Getsockname failed.");
    }
    int dest_port = ntohs(dest_addr.sin_port);
    int src_port = ntohs(source_addr.sin_port);
    char *src_ip = inet_ntoa(source_addr.sin_addr);
    int opcode = opcode_get(packet);
    int block_number;
    char *mode = NULL;
    char *file_name = NULL;
    int error_code;
    char *error_msg = NULL;

    switch (opcode) {
        case RRQ:
        case WRQ:
            file_name = file_name_get(packet);
            mode = mode_get(packet);
            fprintf(stderr, "%s: %s:%d \"%s\" %s",opcode == RRQ ? "RRQ" : "WRQ", src_ip, src_port, file_name, mode);
            display_options(packet);
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
            fprintf(stderr, "OACK: %s:%d", src_ip, src_port);
            display_options(packet);
            break;
        default:
            error_exit("Invalid opcode.");
    }
    packet_pos = 0;
}

void display_options(char *packet) {
    while (packet[packet_pos] != '\0') {
        fprintf(stderr, " %s: %s", packet + packet_pos, packet + packet_pos + strlen(packet + packet_pos) + 1);
        packet_pos += strlen(packet + packet_pos) + 1 + strlen(packet + packet_pos + strlen(packet + packet_pos) + 1) + 1;
    }
    printf("\n");
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
    char packet[DEFAULT_PACKET_SIZE];
    memset(packet, 0, DEFAULT_PACKET_SIZE);
    packet_pos = 0;

    opcode_set(ERROR, packet);
    error_code_set(error_code, packet);
    error_msg_set(error_message, packet);
    empty_byte_insert(packet);

    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
}

//Define functions for setting and getting options.
void option_set(int type, long int value, int order) {
    options[type].flag = true;
    options[type].value = value;
    options[type].order = order;
}

bool option_get_flag(int type) {
    return options[type].flag;
}

int option_get_order(int type) {
    return options[type].order;
}

long int option_get_value(int type) {
    return options[type].value;
}

//Define functions for setting and getting option names and types.
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

void options_set(char *packet) {
    int order = 0;
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (option_get_flag(i) == true) {
            if (option_get_order(i) == order) {
                order++;
                // Copy option name to packet.
                strcpy(packet + packet_pos, option_get_name(i));
                // Increment pointer position in packet.
                packet_pos += strlen(option_get_name(i)) + 1;
                // Copy option value to packet.
                sprintf(packet + packet_pos, "%ld", option_get_value(i));
                // Increment pointer position in packet.
                packet_pos += strlen(packet + packet_pos) + 1;
                i = -1;
            }
        }
    }
}

void options_load(char *packet) {
    char *endptr = NULL;
    char name[MAX_STR_LEN];
    int type;
    long int value;
    static int order = 0;
    while (packet[packet_pos] != '\0') {
        strcpy(name, packet + packet_pos);
        type = option_get_type(name);
        if (type == -1) {
            error_exit("Unsupported option.");
        }
        if (option_get_flag(type) == true) {
            error_exit("Duplicate option.");
        }
        packet_pos += strlen(name) + 1;
        value = strtol(packet + packet_pos, &endptr, 10);
        if (*endptr != '\0') {
            error_exit("Invalid option value.");
        }
        option_set(type, value, order++);
        packet_pos += strlen(packet + packet_pos) + 1;
    }
}

FILE *open_file(char *packet, char *dir_path, struct sockaddr_in addr);