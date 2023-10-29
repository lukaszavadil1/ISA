//
// File: tftp-client.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of TFTP client.
//

#include "../include/tftp-client.h"

/**
*
* @brief Main function of TFTP client.
*
* @param argc Number of command line arguments.
* @param argv Command line arguments array.
*
* @return Program exit code.
*
*/
int main(int argc, char *argv[]) {
    // Server address.
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    size_t server_address_size = sizeof(server_address);

    // Initialize client arguments structure and its members.
    ClientArgs_t *client_args;
    client_args = malloc(sizeof(ClientArgs_t));
    if (client_args == NULL) {
        error_exit("Client args structure malloc failed.");
    }
    init_args(client_args);

    // Default opcode.
    int opcode = WRQ;
    packet_pos = 0;
    int out_block_number = 0;
    char data[DEFAULT_DATA_SIZE] = {0};

    // Parse command line arguments.
    parse_args(argc, argv, client_args, &opcode);

    if (opcode == RRQ) {
        file = fopen(client_args->dest_file_path, "w");
        if (file == NULL) {
            error_exit("Failed to open file.");
        }
    }
    else {
        file = stdin;
    }

    // Packet from client.
    char packet[MAX_PACKET_SIZE];
    memset(&packet, 0, MAX_PACKET_SIZE);
    strcpy(packet, client_args->file_path);

    // Create socket.
    int sock_fd = create_socket();

    // Set server Ipv4 address family.
    server_address.sin_family = AF_INET;

    // Set server port number.
    server_address.sin_port = htons(client_args->port);

    // Convert IP address from text to binary form.
    inet_pton(AF_INET, client_args->host_name, &server_address.sin_addr);

    // Filling packet with opcode, filename and mode.
    opcode_set(opcode, packet);
    if (opcode == WRQ) {
        file_name_set(client_args->dest_file_path, packet);
    }
    else {
        file_name_set(client_args->file_path, packet);
    }
    empty_byte_insert(packet);
    mode_set(1, packet); // Set octet mode.
    empty_byte_insert(packet);

    // Send data to the server.
    if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM,
               (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        error_exit("Sendto failed on client side.");
    }

    memset(&packet, 0, MAX_PACKET_SIZE);
    packet_pos = 0;

    if (opcode == RRQ) {
        while (true) {
            if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
                error_exit("Recvfrom failed on client side.");
            }

            opcode = opcode_get(packet);
            packet_pos = 0;
            if (opcode == ERROR) {
                display_message(sock_fd, server_address, packet);
                fclose(file);
                exit(EXIT_FAILURE);
            }
            packet_pos = 0;
            handle_data(packet, ++out_block_number);

            strcpy(data, data_get(packet));
            fputs(data, file);
            packet_pos = 0;

            display_message(sock_fd, server_address, packet);

            memset(&packet, 0, MAX_PACKET_SIZE);

            opcode_set(ACK, packet);
            block_number_set(out_block_number, packet);

            if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM,
                       (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                error_exit("Sendto failed on client side.");
            }

            memset(&packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            if (strlen(data) < DEFAULT_DATA_SIZE) {
                fclose(file);
                break;
            }
        }
    }
    else if (opcode == WRQ) {
        if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
            error_exit("Recvfrom failed on client side.");
        }
        opcode = opcode_get(packet);
        packet_pos = 0;
        if(opcode == ERROR) {
            display_message(sock_fd, server_address, packet);
            fclose(file);
            exit(EXIT_FAILURE);
        }
        handle_ack(packet, 0);
        display_message(sock_fd, server_address, packet);

        memset(&packet, 0, MAX_PACKET_SIZE);

        while(true) {
            printf("> ");
            fgets(data, DEFAULT_DATA_SIZE, stdin);
            send_data_packet(sock_fd, server_address, ++out_block_number, data);

            if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
                error_exit("Recvfrom failed on client side.");
            }

            handle_ack(packet, out_block_number);
            display_message(sock_fd, server_address, packet);

            memset(&packet, 0, MAX_PACKET_SIZE);

            if (strlen(data) < DEFAULT_DATA_SIZE) {
                break;
            }
        }
    }
    else {
        error_exit("Invalid request.");
    }
    return EXIT_SUCCESS;
}

void init_args(ClientArgs_t *client_args) {
    client_args->host_name = malloc(MAX_STR_LEN);
    client_args->port = DEFAULT_PORT_NUM;
    client_args->file_path = malloc(MAX_STR_LEN);
    client_args->dest_file_path = malloc(MAX_STR_LEN);
    if (client_args->host_name == NULL || client_args->file_path == NULL || client_args->dest_file_path == NULL) {
        error_exit("Client args member malloc failed.");
    }
}

void free_args(ClientArgs_t *client_args) {
    free(client_args->host_name);
    free(client_args->file_path);
    free(client_args->dest_file_path);
    free(client_args);
}

void parse_args(int argc, char *argv[], ClientArgs_t *client_args, int *opcode) {
    if (argc == 1) {
        display_client_help();
        exit(EXIT_SUCCESS);
    }
    if (argc > 9 || argc < 5) { 
        error_exit("Invalid number of arguments.");
    }
    int opt;
    bool h_flag = false, p_flag = false, f_flag = false, t_flag = false;
    while ((opt = getopt(argc, argv, ":h:p:f:t:")) != -1) {
        switch (opt) {
            case 'h':
                if (h_flag) {
                    error_exit("Duplicate flag -h.");
                }
                strcpy(client_args->host_name, optarg);
                h_flag = true;
                break;
            case 'p':
                if (p_flag) {
                    error_exit("Duplicate flag -p.");
                }
                int port = parse_port(optarg);
                client_args->port = port;
                p_flag = true;
                break;
            case 'f':
                if (f_flag) {
                    error_exit("Duplicate flag -f.");
                }
                strcpy(client_args->file_path, optarg);
                *opcode = RRQ;
                f_flag = true;
                break;
            case 't':
                if (t_flag) {
                    error_exit("Duplicate flag -t.");
                }
                strcpy(client_args->dest_file_path, optarg);
                t_flag = true;
                break;
            case ':':
                error_exit("Missing argument.");
                break;
            default:
                error_exit("Argument error.");
        }
        if (argv[optind] != NULL) {
            if ((strcmp(argv[optind], "-h") != 0) && (strcmp(argv[optind], "-p") != 0) && (strcmp(argv[optind], "-f") != 0) && (strcmp(argv[optind], "-t") != 0)) {
                error_exit("Flag must have only one argument.");
            }
        }
    }
    if (h_flag == false) {
        error_exit("Missing flag -h.");
    }
    if (t_flag == false) {
        error_exit("Missing flag -t.");
    }
}