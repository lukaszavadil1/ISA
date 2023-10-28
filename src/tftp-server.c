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
    char packet[MAX_PACKET_SIZE];      
    memset(&packet, 0, MAX_PACKET_SIZE);
    
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

        memset(packet, 0, MAX_PACKET_SIZE);
        packet_pos = 0;

        if ((recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                      (socklen_t *) &client_address_size)) < 0) {
            error_exit("Recvfrom failed on server side.");
        }

        pid = fork();
        if (pid < 0) {
            error_exit("Server fork failed.");
        }
        else if (pid == 0) {
            // Packet data.
            int opcode;
            char file_name[MAX_STR_LEN] = {0};
            char data[DEFAULT_DATA_SIZE] = {0};
            char mode[MAX_STR_LEN] = {0};
            char line[DEFAULT_DATA_SIZE] = {0};

            // Block numbers for sent and received packets.
            int out_block_number = 0;

            // Pointer to the current position in packet.
            packet_pos = 0;

            sock_fd = create_socket();
            server_addr.sin_port = htons(0);

            handle_client_request(packet);
            display_message(sock_fd, client_address, packet); 

            opcode = opcode_get(packet);
            strcpy(file_name, file_name_get(packet));
            strcpy(mode, mode_get(packet));

            file = fopen(file_name, "r+");
            if (file == NULL) {
                error_exit("Failed to open file.");
            }           

            memset(packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            if (opcode == RRQ) {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    // Send OACK packet.
                    opcode_set(OACK, packet);
                    options_set(packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }
                    
                    handle_ack(packet, 0);
                    display_message(sock_fd, client_address, packet);

                    memset(packet, 0, MAX_PACKET_SIZE);
                }
                while(true) {
                    opcode_set(DATA, packet);
                    out_block_number++;
                    block_number_set(out_block_number, packet);
                    while(fgets(line, DEFAULT_DATA_SIZE, file) != NULL) {
                        strcat(data, line);
                    }
                    data_set(data, packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }

                    handle_ack(packet, out_block_number);
                    display_message(sock_fd, client_address, packet);

                    memset(packet, 0, MAX_PACKET_SIZE);

                    if (strlen(data) < DEFAULT_DATA_SIZE) {
                        fclose(file);
                        break;
                    }
                }
            }
            else {
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

                memset(packet, 0, MAX_PACKET_SIZE);
                packet_pos = 0;

                while (true) {
                    if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }

                    out_block_number++;
                    handle_data(packet, out_block_number);

                    // Stdin -> server -> file
                    strcpy(data, data_get(packet));
                    fputs(data, file);
                    packet_pos = 0;

                    display_message(sock_fd, client_address, packet);

                    memset(packet, 0, MAX_PACKET_SIZE);

                    opcode_set(ACK, packet);
                    block_number_set(out_block_number, packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    if (strlen(data) < DEFAULT_DATA_SIZE) {
                        fclose(file);
                        break;
                    }
                }
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