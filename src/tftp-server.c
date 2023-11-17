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
    // Server and client address structures.                 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    struct sockaddr_in client_address;                  
    size_t client_address_size = sizeof(client_address);

    // Packet buffer for incoming and outgoing packets.
    char packet[DEFAULT_PACKET_SIZE];      
    memset(packet, 0, DEFAULT_PACKET_SIZE);
    
    // Initialize server arguments structure and its members.
    ServerArgs_t *server_args;
    server_args = malloc(sizeof(ServerArgs_t));
    if (server_args == NULL) {
        error_exit("Server args structure malloc failed.");
    }
    init_args(server_args);

    // Parse command line arguments.
    parse_args(argc, argv, server_args);

    int sock_fd = init_socket(server_args->port, &server_addr);

    // Bind server to all interfaces.
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error_exit("Bind failed.");
    }

    // Process id
    pid_t pid;

    // Listen for incoming client connections.
    while (true) {
        // Handle SIGINT signal.
        signal(SIGINT, sigint_handler);

        memset(packet, 0, DEFAULT_PACKET_SIZE);
        packet_pos = 0;

        // Listen for incoming request packets.
        if ((recvfrom(sock_fd, (char *)packet, DEFAULT_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                      (socklen_t *) &client_address_size)) < 0) {
            error_exit("Recvfrom failed on server side.");
        }

        pid = fork();
        if (pid < 0) {
            error_exit("Server fork failed.");
        }
        else if (pid == 0) {
            // Request packet attributes.
            int opcode;
            char file_name[MAX_STR_LEN] = {0};
            char mode[MAX_STR_LEN] = {0};
            char path[2*MAX_STR_LEN] = {0};
            int out_block_number = 0;

            // Pointer to the current position in packet.
            packet_pos = 0;
            // Last packet flag.
            last = false;

            // Initialize socket.
            sock_fd = init_socket(0, &server_addr);

            // Generate random port number to respond from.
            server_addr.sin_port = htons(0);

            // Handle request packet from client and display it.
            handle_request_packet(packet);
            for (int i = 0; i < DEFAULT_PACKET_SIZE; i++) {
                printf("%c ", packet[i]);
            }
            printf("\n");
            display_message(sock_fd, client_address, packet); 

            opcode = opcode_get(packet);
            strcpy(file_name, file_name_get(packet));
            strcpy(mode, mode_get(packet));
            strcpy(path, server_args->dir_path);
            strcat(path, "/");
            strcat(path, file_name);

            //file = open_file(packet, server_args->dir_path, client_address);
            if (opcode == WRQ) {
                if (access(path, F_OK) != -1) {
                    send_error_packet(sock_fd, client_address, 6, "File already exists.");
                    exit(EXIT_FAILURE);
                }
                file = fopen(path, "w");
            }
            else if (opcode == RRQ) {
                if (strcmp(mode, "netascii") == 0) {
                    file = fopen(path, "r");
                }
                else if (strcmp(mode, "octet") == 0) {
                    file = fopen(path, "rb");
                }
                else {
                    send_error_packet(sock_fd, client_address, 4, "Illegal TFTP operation.");
                    exit(EXIT_FAILURE);
                }
            }
            else {
                send_error_packet(sock_fd, client_address, 4, "Illegal TFTP operation.");
                exit(EXIT_FAILURE);
            }
            if (file == NULL) {
                send_error_packet(sock_fd, client_address, 1, "File not found.");
                exit(EXIT_FAILURE);
            }           

            memset(packet, 0, DEFAULT_PACKET_SIZE);
            packet_pos = 0;

            if (opcode == RRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    send_oack_packet(sock_fd, client_address);
                    if (recvfrom(sock_fd, (char *)packet, DEFAULT_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }
                    
                    handle_ack(packet, 0);
                    display_message(sock_fd, client_address, packet);

                    memset(packet, 0, DEFAULT_PACKET_SIZE);
                }
                while(true) {
                    last = send_data_packet(sock_fd, client_address, ++out_block_number, file);
                    if (recvfrom(sock_fd, (char *)packet, DEFAULT_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }
                    handle_ack(packet, out_block_number);
                    display_message(sock_fd, client_address, packet);
                    if (last == true) {
                        fclose(file);
                        break;
                    }
                }
            }
            else if (opcode == WRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    opcode_set(OACK, packet);
                    options_set(packet);
                }
                else {
                    opcode_set(ACK, packet);
                    block_number_set(0, packet);
                }

                if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                memset(packet, 0, DEFAULT_PACKET_SIZE);
                packet_pos = 0;

                while (true) {
                    if (recvfrom(sock_fd, (char *)packet, DEFAULT_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }

                    last = handle_data(packet, ++out_block_number, file);
                    packet_pos = 0;
                    display_message(sock_fd, client_address, packet);
                    memset(packet, 0, DEFAULT_PACKET_SIZE);

                    opcode_set(ACK, packet);
                    block_number_set(out_block_number, packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    memset(packet, 0, DEFAULT_PACKET_SIZE);
                    packet_pos = 0;

                    if (last == true) {
                        fclose(file);
                        break;
                    }
                }
            }
            else {
                send_error_packet(sock_fd, client_address, 4, "Illegal TFTP operation.");
                exit(EXIT_FAILURE);
            }
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
    if (argc == 2) {
        strcpy(server_args->dir_path, argv[1]);
        DIR *dir = opendir(server_args->dir_path);
        if (dir == NULL) {
            error_exit("Failed to open directory.");
        }
        closedir(dir);
        return;
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
        DIR *dir = opendir(server_args->dir_path);
        if (dir == NULL) {
            error_exit("Failed to open directory.");
        }
        closedir(dir);
    } 
    else {
        error_exit("Missing directory path.");
    }
}