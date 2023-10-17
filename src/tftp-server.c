//
// File: tfpt-server.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of TFTP server.
//

#include "../include/tftp-server.h"

/**
*
* @brief Main function of TFTP server.
*
* @param argc Number of command line arguments.
* @param argv Command line arguments array.
*
* @return Program exit code.
*
*/
int main(int argc, char *argv[]) {
    // Server address.                           
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    // Packet from client.
    char packet[MAX_PACKET_SIZE];      
    memset(&packet, 0, MAX_PACKET_SIZE);

    // Client address and its size.           
    struct sockaddr client_address;                  
    size_t client_address_size = sizeof(client_address);
    
    // Initialize server arguments structure and its members.
    ServerArgs_t *server_args;
    server_args = malloc(sizeof(ServerArgs_t));
    if (server_args == NULL) {
        error_exit("Server args structure malloc failed.");
    }
    init_args(server_args);

    // Parse command line arguments.
    parse_args(argc, argv, server_args);

    int sock_fd = create_socket();

    // Set Ipv4 address family.
    server_addr.sin_family = AF_INET;

    // Set port number.
    server_addr.sin_port = htons(server_args->port);

    // Bind to all interfaces.
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to the server address and port.
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error_exit("Failed to bind server socket.");
    }

    // Listen for incoming client connections.
    while (true) {
        if ((recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                      (socklen_t *) &client_address_size)) < 0) {
            error_exit("Recvfrom failed on server side.");
        }
        printf("Received packet.\n");
        printf("Packet data: %s\n", packet);
        memset(&packet, 0, MAX_PACKET_SIZE);
        if (sendto(sock_fd, packet, MAX_PACKET_SIZE, MSG_CONFIRM, (const struct sockaddr *)&client_address,
                   client_address_size) < 0) {
            error_exit("Sendto failed on server side.");
        }
        printf("Sent ack.\n");
    }

    // Free server arguments structure and its members.
    free_args(server_args);
    return 0;
}

void init_args(ServerArgs_t *server_args) {
    server_args->port = DEFAULT_PORT_NUM;
    server_args->dir_path = malloc(MAX_STR_LEN);
    if (server_args->dir_path == NULL) {
        error_exit("Server args dir path malloc failed.");
    }
}

void free_args(ServerArgs_t *server_args) {
    free(server_args->dir_path);
    free(server_args);
}

void parse_args(int argc, char *argv[], ServerArgs_t *server_args) {
    if (argc == 1) {
        display_server_help();
        exit(EXIT_SUCCESS);
    }
    if (argc > 4 || argc < 2) { 
        error_exit("Invalid number of arguments.");
    }
    int opt;
    bool p_flag = false;
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                if (p_flag) {
                    error_exit("Duplicate flag -p.");
                }
                server_args->port = parse_port(optarg);
                p_flag = true;
                break;
            default:
                error_exit("Invalid option.");
        }
    }
    if (optind < argc) {
        strcpy(server_args->dir_path, argv[optind]);
    } 
    else {
        error_exit("Missing directory path.");
    }
}
