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
    int in_block_number = 0;
    char data[MAX_PACKET_SIZE];
    FILE *file;

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

    printf("Sending packet...\n\n");
    printf("#########################################\n");
    printf("Packet info\n\n");
    printf("Opcode: %s\n", opcode_to_str(opcode));
    printf("File name: %s\n", opcode == WRQ ? client_args->dest_file_path : client_args->file_path);
    printf("Mode: octet\n");
    printf("#########################################\n\n");

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
            if (opcode != DATA) {
                error_exit("Invalid opcode received.");
            }

            in_block_number = block_number_get(packet);
            if (in_block_number != out_block_number + 1) {
                error_exit("Invalid block number received.");
            }
            out_block_number++;

            strcpy(data, data_get(packet));

            fputs(data, file);

            printf("Received packet...\n\n");
            print_data_packet(in_block_number, data);

            memset(&packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            opcode_set(ACK, packet);
            block_number_set(out_block_number, packet);

            if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM,
                       (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                error_exit("Sendto failed on client side.");
            }

            printf("Sent packet...\n\n");
            print_ack_packet(out_block_number);

            memset(&packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            if (strlen(data) < MAX_PACKET_SIZE - 4) {
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
        if (opcode != ACK) {
            error_exit("Invalid opcode received.");
        }

        in_block_number = block_number_get(packet);
        if (in_block_number != 0) {
            error_exit("Invalid block number received.");
        }

        printf("Received packet...\n\n");
        print_ack_packet(in_block_number);

        memset(&packet, 0, MAX_PACKET_SIZE);
        packet_pos = 0;

        while(1) {
            opcode_set(DATA, packet);
            out_block_number++;
            block_number_set(out_block_number, packet);

            printf("> ");
            fgets(data, MAX_PACKET_SIZE, stdin);

            data_set(data, packet);

            if (sendto(sock_fd, packet, packet_pos, MSG_CONFIRM,
                       (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
                error_exit("Sendto failed on client side.");
            }

            opcode = opcode_get(packet);
            if (opcode != DATA) {
                error_exit("Invalid opcode received.");
            }

            printf("Sending packet...\n\n");
            print_data_packet(out_block_number, data);

            memset(&packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            if (recvfrom(sock_fd, (char *)packet, MAX_PACKET_SIZE, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
                error_exit("Recvfrom failed on client side.");
            }

            opcode = opcode_get(packet);
            if (opcode != ACK) {
                error_exit("Invalid opcode received.");
            }

            in_block_number = block_number_get(packet);
            if (in_block_number != out_block_number) {
                error_exit("Invalid block number received.");
            }

            printf("Received packet...\n\n");
            print_ack_packet(in_block_number);

            memset(&packet, 0, MAX_PACKET_SIZE);
            packet_pos = 0;

            if (strlen(data) < MAX_PACKET_SIZE - 4) {
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