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
    struct sockaddr client_address;                  
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
            int opcode = 0;
            char file_name[MAX_STR_LEN];
            char mode[MAX_STR_LEN];
            char data[MAX_PACKET_SIZE];

            // Block numbers for sent and received packets.
            int out_block_number = 0, in_block_number = 0;

            // Pointer to the current position in packet.
            packet_pos = 0;

            sock_fd = create_socket();

            handle_client_request(packet, &opcode, file_name, mode);
            print_client_request(opcode, file_name, mode);

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
                    
                    printf("Sending packet...\n\n");
                    print_oack_packet();

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }
                    if (opcode_get(packet) != ACK) {
                        error_exit("Invalid opcode, server expected ACK.");
                    }
                    if (block_number_get(packet) != 0) {
                        error_exit("Invalid block number, server expected 0.");
                    }
                    printf("Received packet...\n\n");
                    print_ack_packet(in_block_number);

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
                while(true) {
                    opcode_set(DATA, packet);
                    out_block_number++;
                    block_number_set(out_block_number, packet);
                    printf("> ");
                    fgets(data, MAX_PACKET_SIZE, stdin);
                    data_set(data, packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    printf("Sending packet...\n\n");
                    print_data_packet(out_block_number, data);

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&client_address,
                                (socklen_t *) &client_address_size) < 0) {
                        error_exit("Recvfrom failed on server side.");
                    }

                    opcode = opcode_get(packet);
                    if (opcode != ACK) {
                        error_exit("Invalid opcode, server expected ACK.");
                    }

                    in_block_number = block_number_get(packet);
                    if (in_block_number != out_block_number) {
                        error_exit("Invalid block number.");
                    }

                    printf("Received packet...\n\n");
                    print_ack_packet(in_block_number);

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
                }
            }
            else {
                if (options[TIMEOUT].flag || options[TSIZE].flag || options[BLKSIZE].flag) {
                    opcode_set(OACK, packet);
                    options_set(packet);
                    print_oack_packet();
                }
                else {
                    opcode_set(ACK, packet);
                    block_number_set(0, packet);
                    print_ack_packet(out_block_number);
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

                    opcode = opcode_get(packet);
                    if (opcode != DATA) {
                        error_exit("Invalid opcode, server expected DATA.");
                    }

                    in_block_number = block_number_get(packet);
                    out_block_number++;
                    if (in_block_number != out_block_number) {
                        error_exit("Invalid block number.");
                    }

                    printf("Received packet...\n\n");
                    print_data_packet(in_block_number, data_get(packet));

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;

                    opcode_set(ACK, packet);
                    block_number_set(out_block_number, packet);

                    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM, (struct sockaddr *)&client_address,
                                client_address_size) < 0) {
                        error_exit("Sendto failed on server side.");
                    }

                    printf("Sending packet...\n\n");
                    print_ack_packet(out_block_number);

                    memset(packet, 0, MAX_PACKET_SIZE);
                    packet_pos = 0;
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