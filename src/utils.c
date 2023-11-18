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
    if (strlen(file_name) > MAX_FILE_NAME_LEN - 1) {
        error_exit("File name too long.");
    }
    strncpy(packet_pos + packet, file_name, MAX_FILE_NAME_LEN);
    packet_pos += strnlen(file_name, MAX_FILE_NAME_LEN);
}

char *file_name_get(char *packet) {
    char *file_name = calloc(MAX_FILE_NAME_LEN, sizeof(char));
    // Get file name from packet.
    file_name = packet_pos + packet;
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
    char *mode = calloc(MAX_MODE_LEN, sizeof(char));
    // Get mode from packet.
    mode = packet_pos + packet;
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
    // Get block number from packet.
    memcpy(&block_number, packet + packet_pos, BLOCK_NUMBER_SIZE);
    // Convert block number to host byte order.
    block_number = ntohs(block_number);
    packet_pos += BLOCK_NUMBER_SIZE;
    return block_number;
}

void data_set(char *packet, FILE *file) {
    int c;
    while ((c = fgetc(file)) != EOF) {
        packet[packet_pos++] = (char)c;
        if (packet_pos == options[BLKSIZE].value + 4) {
            break;
        }
    }
}

char *data_get(char *packet) {
    char *data = packet + packet_pos;
    packet_pos += strnlen(data, options[BLKSIZE].value + 4);
    return data;
}

void error_code_set(int error_code, char *packet) {
    // Save error code inside packet in network byte order.
    *(int *)(packet + packet_pos) = htons(error_code);
    packet_pos += ERROR_CODE_SIZE;
}

int error_code_get(char *packet) {
    int error_code;
    // Get error code from packet.
    memcpy(&error_code, packet + packet_pos, ERROR_CODE_SIZE);
    // Convert error code to host byte order.
    error_code = ntohs(error_code);
    packet_pos += ERROR_CODE_SIZE;
    return error_code;
}

void error_msg_set(char *error_message, char *packet) {
    // Save error message inside packet.
    strcpy(packet + packet_pos, error_message);
    packet_pos += strlen(error_message);
}

char *error_msg_get(char *packet) {
    char *error_msg = malloc(MAX_STR_LEN);
    // Get error message from packet.
    error_msg = packet + packet_pos;
    packet_pos += strlen(error_msg);
    return error_msg;
}

void option_set(int type, long int value, int order) {
    switch (type) {
        case TIMEOUT:
            if (value < 1 || value > 255) {
                error_exit("Invalid timeout value.");
            }
            break;
        case TSIZE:
            if (value < 0 || value > 428998656) {
                error_exit("Invalid tsize value.");
            }
            break;
        case BLKSIZE:
            if (value < 8 || value > 65464) {
                error_exit("Invalid blksize value.");
            }
            break;
        default:
            error_exit("Invalid option type.");
    }
    options[type].flag = true;
    options[type].value = value;
    options[type].order = order;
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

long int option_get_value(int type) {
    return options[type].value;
}

int option_get_order(int type) {
    return options[type].order;
}

bool option_get_flag(int type) {
    return options[type].flag;
}

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

void options_load(char *packet) {
    char *endptr = NULL;
    char name[MAX_STR_LEN];
    int type;
    long int value;
    static int order = 0;
    options[TIMEOUT] = (Option_t){false, 0, -1};
    options[TSIZE] = (Option_t){false, 0, -1};
    options[BLKSIZE] = (Option_t){false, 0, -1};
    while (packet[packet_pos] != '\0') {
        strncpy(name, packet + packet_pos, MAX_STR_LEN - 1);
        type = option_get_type(name);
        if (type == -1) {
            error_exit("Unsupported option.");
        }
        if (option_get_flag(type) == true) {
            error_exit("Duplicate option.");
        }
        packet_pos += strnlen(name, MAX_STR_LEN) + 1;
        value = strtol(packet + packet_pos, &endptr, 10);
        if (*endptr != '\0') {
            error_exit("Invalid option value.");
        }
        option_set(type, value, order++);
        packet_pos += strlen(packet + packet_pos) + 1;
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

void handle_request_packet(char *packet) {
    int opcode;
    char file_name[MAX_FILE_NAME_LEN + 1];
    char mode[MAX_MODE_LEN + 1];

    opcode = opcode_get(packet);
    if (opcode != RRQ && opcode != WRQ) {
        error_exit("Invalid opcode, server expected RRQ or WRQ.");
    }
    
    strncpy(file_name, file_name_get(packet), MAX_FILE_NAME_LEN);
    if (strlen(file_name) == 0) {
        error_exit("File name cannot be empty.");
    }
    strncpy(mode, mode_get(packet), MAX_MODE_LEN);
    if (strcmp(mode, "octet") != 0 && strcmp(mode, "netascii") != 0) {
        error_exit("Unsupported mode.");
    }
    options_load(packet);
    if (packet_pos == REQUEST_PACKET_SIZE) {
        error_exit("Request packet too long.");
    }
    packet_pos = 0;
}

void send_request_packet(int socket, struct sockaddr_in dest_addr, int opcode, char *file_name) {
    char packet[REQUEST_PACKET_SIZE];
    memset(packet, 0, REQUEST_PACKET_SIZE);

    // Set packet attributes.
    opcode_set(opcode, packet);
    file_name_set(file_name, packet);
    empty_byte_insert(packet);
    // Octet mode is default.
    mode_set(OCTET, packet);
    empty_byte_insert(packet);
    // Set options.
    option_set(BLKSIZE, 80, 1);
    option_set(TIMEOUT, 5, 0);
    options_set(packet);
    
    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    packet_pos = 0;
}

void handle_ack_packet(char *packet, int expected_block_number) {
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

void handle_oack_packet(char *packet) {
    int opcode;

    opcode = opcode_get(packet);
    if (opcode != OACK) {
        error_exit("Invalid opcode, server expected OACK.");
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

bool handle_data_packet(char *packet, int expected_block_number, FILE *file) {
    int opcode, block_number;
    char data[options[BLKSIZE].value];

    opcode = opcode_get(packet);
    if (opcode != DATA) {
        error_exit("Invalid opcode, server expected DATA.");
    }
    block_number = block_number_get(packet);
    if (block_number != expected_block_number) {
        error_exit("Invalid data block number.");
    }
    memcpy(data, data_get(packet), options[BLKSIZE].value + 4);
    for (size_t i = 0; i < strnlen(data, options[BLKSIZE].value + 4); i++) {
        fputc(data[i], file);
    }
    if (strnlen(data, options[BLKSIZE].value) < (long unsigned int)options[BLKSIZE].value) {
        packet_pos = 0;
        return true;
    }
    packet_pos = 0;
    return false;
}

bool send_data_packet(int socket, struct sockaddr_in dest_addr, int block_number, FILE *file) {
    char packet[options[BLKSIZE].value + 4];
    memset(packet, 0, options[BLKSIZE].value + 4);

    opcode_set(DATA, packet);
    block_number_set(block_number, packet);
    data_set(packet, file);
    if (sendto(socket, packet, packet_pos, MSG_CONFIRM, (const struct sockaddr *)&dest_addr, (socklen_t)sizeof(dest_addr)) < 0) {
        error_exit("Sendto failed.");
    }
    if (packet_pos < options[BLKSIZE].value + 4) {
        packet_pos = 0;
        return true;
    }
    packet_pos = 0;
    return false;
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

FILE *open_file(int socket, char *packet, char *dir_path, struct sockaddr_in addr) {
    int opcode;
    char file_name[MAX_FILE_NAME_LEN + 1];
    char mode[MAX_MODE_LEN + 1];
    char full_path[MAX_FILE_NAME_LEN + MAX_DIR_PATH_LEN + 2];
    FILE *file = NULL;
    opcode = opcode_get(packet);
    strncpy(file_name, file_name_get(packet), MAX_FILE_NAME_LEN);
    strncpy(mode, mode_get(packet), MAX_MODE_LEN);
    strncpy(full_path, dir_path, MAX_DIR_PATH_LEN);
    strcat(full_path, "/");
    strcat(full_path, file_name);

    if (opcode == WRQ) {
        if (access(full_path, F_OK) != -1) {
            send_error_packet(socket, addr, 6, "File already exists.");
            exit(EXIT_FAILURE);
        }
        file = fopen(full_path, "w");
    }
    else if (opcode == RRQ) {
        if (strcmp(mode, "netascii") == 0) {
            file = fopen(full_path, "r");
        }
        else if (strcmp(mode, "octet") == 0) {
            file = fopen(full_path, "rb");
        }
        else {
            send_error_packet(socket, addr, 4, "Illegal TFTP operation.");
            exit(EXIT_FAILURE);
        }
    }
    else {
        send_error_packet(socket, addr, 4, "Illegal TFTP operation.");
        exit(EXIT_FAILURE);
    }
    if (file == NULL) {
        send_error_packet(socket, addr, 1, "File not found.");
        exit(EXIT_FAILURE);
    }
    packet_pos = 0;
    return file;
}

// Mention in docs: https://stackoverflow.com/questions/59658328/implicit-declaration-of-strnlen-in-freertos
size_t strnlen(const char *s, size_t maxlen) {
    size_t len = 0;
    if(s)
        for(char c = *s; (len < maxlen && c != '\0'); c = *++s) len++;
    return len;
}