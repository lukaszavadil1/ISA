//
// File: tfpt-server.c
//
// Author: Lukáš Zavadil (xzavad20)
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
    char *packet = calloc(DEFAULT_PACKET_SIZE, sizeof(char));      
    
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

        memset(packet, 0, REQUEST_PACKET_SIZE);
        packet_pos = 0;

        // Listen for incoming request packets.
        if ((recvfrom(sock_fd, (char *)packet, REQUEST_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
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
            int out_block_number = 0;
            int recvfrom_size;

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
            display_message(sock_fd, client_address, packet); 

            file = open_file(sock_fd, packet, server_args->dir_path, client_address);
            opcode = opcode_get(packet);
            if (opcode == RRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    send_oack_packet(sock_fd, client_address);
                    if (recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&client_address, (socklen_t *) &client_address_size) < 0) {
                        send_error_packet(sock_fd, client_address, 0, "Recvfrom failed on server side.");
                    }
                    handle_ack_packet(packet, 0);
                    display_message(sock_fd, client_address, packet);
                }
                packet = realloc(packet, options[BLKSIZE].value + 4);
                while(true) {
                    memset(packet, 0, options[BLKSIZE].value + 4);
                    last = send_data_packet(sock_fd, client_address, ++out_block_number, file);
                    if (recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        send_error_packet(sock_fd, client_address, 0, "Recvfrom failed on server side.");
                    }
                    handle_ack_packet(packet, out_block_number);
                    display_message(sock_fd, client_address, packet);

                    if (last == true) {
                        fclose(file);
                        break;
                    }
                }
            }
            else if (opcode == WRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    send_oack_packet(sock_fd, client_address);
                }
                else {
                    send_ack_packet(sock_fd, client_address, 0);
                }
                packet = realloc(packet, options[BLKSIZE].value + 4);
                while (true) {
                    memset(packet, 0, options[BLKSIZE].value + 4);
                    if ((recvfrom_size = recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size)) < 0) {
                        fclose(file);
                        send_error_packet(sock_fd, client_address, 0, "Recvfrom failed on server side.");
                    }
                    handle_data_packet(packet, ++out_block_number, file, recvfrom_size);
                    display_message(sock_fd, client_address, packet);
                    memset(packet, 0, options[BLKSIZE].value + 4);

                    send_ack_packet(sock_fd, client_address, out_block_number);
                    if (recvfrom_size < options[BLKSIZE].value + 4) {
                        fclose(file);
                        break;
                    }
                }
            }
            else {
                send_error_packet(sock_fd, client_address, 4, "Expected RRQ or WRQ.");
                exit(EXIT_FAILURE);
            }
            shutdown(sock_fd, SHUT_RDWR);
            close(sock_fd);
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