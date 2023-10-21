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

    // Process id
    pid_t pid;

    // Listen for incoming client connections.
    while (true) {
        
        signal(SIGINT, sigint_handler);

        if ((recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                      (socklen_t *) &client_address_size)) < 0) {
            error_exit("Recvfrom failed on server side.");
        }

        pid = fork();
        if (pid < 0) {
            error_exit("Server fork failed.");
        }
        else if (pid == 0) {
            char file_name[MAX_STR_LEN];
            char mode[MAX_STR_LEN];
            char *opcode_str;
            int client_block_num = 0;
            int server_block_num = 0;
            int opcode = 0;
            packet_pos = 0;

            handle_client_request(packet, &opcode, file_name, mode);
            print_client_request(opcode, file_name, mode);

            memset(packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;
            if (opcode == RRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    // Send OACK packet.
                    opcode_set(OACK, packet);
                    opcode_str = opcode_to_str(OACK);
                    options_set(packet);
                    printf("#########################################\n");
                    printf("Packet info: Sending packet to client\n\n");
                    printf("Opcode: %s\n", opcode_str);
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
                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }
                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
                else {
                    // Send ACK packet.
                    opcode_set(ACK, packet);
                    opcode_str = opcode_to_str(ACK);
                    printf("#########################################\n");
                    printf("Packet info: Sending packet to client\n\n");
                    printf("Opcode: %s\n", opcode_str);
                    printf("#########################################\n\n");
                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }
                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
            }
            else if (opcode == WRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    // Send OACK packet.
                    opcode_set(OACK, packet);
                    options_set(packet);
                    opcode_str = opcode_to_str(OACK);
                    printf("#########################################\n");
                    printf("Packet info: Sending packet to client\n\n");
                    printf("Opcode: %s\n", opcode_str);
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
                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }
                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
                else {
                    // Send ACK packet.
                    opcode_set(ACK, packet);
                    opcode_str = opcode_to_str(ACK);
                    printf("#########################################\n");
                    printf("Packet info: Sending packet to client\n\n");
                    printf("Opcode: %s\n", opcode_str);
                    printf("#########################################\n\n");
                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }
                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
            }
            else {
                error_exit("Invalid opcode, server expected RRQ or WRQ.");
            }
        }
        else {

        }
    }
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

void handle_client_request(char *packet, int *opcode, char *file_name, char *mode) {
    *opcode = opcode_get(packet);
    if (*opcode != RRQ && *opcode != WRQ) {
        error_exit("Invalid opcode, server expected RRQ or WRQ.");
    }
    strcpy(file_name, file_name_get(packet));
    strcpy(mode, mode_get(packet));
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