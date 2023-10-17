//
// File: utils.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of helper functions.
//

#include "../include/utils.h"

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
    free(endptr);
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