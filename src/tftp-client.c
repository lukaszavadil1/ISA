//
// File: tftp-client.c
//
// Author: Lukáš Zavadil (xzavad20)
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

    // Packet attributes.
    char *packet = calloc(DEFAULT_PACKET_SIZE, sizeof(char));
    opcode = WRQ;
    out_block_number = 0;
    packet_pos = 0;
    last = false;
    int recvfrom_size;

    // Initialize client arguments structure and its members.
    ClientArgs_t *client_args;
    client_args = malloc(sizeof(ClientArgs_t));
    if (client_args == NULL) {
        error_exit("Client args structure malloc failed.");
    }
    init_args(client_args);

    // Parse command line arguments.
    parse_args(argc, argv, client_args, &opcode);

    // Handle client data stream.
    file = client_data_stream(opcode, client_args);

    // Create socket.
    int sock_fd = init_socket(client_args->port, &server_address);

    // Convert IP address from text to binary form.
    inet_pton(AF_INET, client_args->host_name, &server_address.sin_addr);    

    if (opcode == RRQ) {
        send_request_packet(sock_fd, server_address, RRQ, client_args->file_path);
        packet = realloc(packet, options[BLKSIZE].value + 4);
        while (true) {
            memset(packet, 0, options[BLKSIZE].value + 4);
            if ((recvfrom_size = recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size)) < 0) {
                error_exit("Recvfrom failed on client side.");
            }
            opcode = opcode_get(packet);
            packet_pos = 0;
            switch(opcode) {
                case DATA:
                    handle_data_packet(packet, ++out_block_number, file, recvfrom_size);
                    break;
                case ERROR:
                    display_message(sock_fd, server_address, packet);
                    fclose(file);
                    exit(EXIT_FAILURE);
                case OACK:
                    handle_oack_packet(packet);
                    recvfrom_size = options[BLKSIZE].value + 4;
                    break;
                default:
                    error_exit("Invalid opcode.");
            }
            display_message(sock_fd, server_address, packet);
            memset(packet, 0, options[BLKSIZE].value + 4);

            send_ack_packet(sock_fd, server_address, out_block_number);

            if (recvfrom_size < options[BLKSIZE].value + 4) {
                fclose(file);
                break;
            }
        }
    }
    else if (opcode == WRQ) {
        send_request_packet(sock_fd, server_address, WRQ, client_args->dest_file_path);
        packet = realloc(packet, options[BLKSIZE].value + 4);
        if (recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
            error_exit("Recvfrom failed on client side.");
        }
        opcode = opcode_get(packet);
        packet_pos = 0;
        switch (opcode) {
            case ACK:
                handle_ack_packet(packet, 0);
                break;
            case ERROR:
                display_message(sock_fd, server_address, packet);
                fclose(file);
                exit(EXIT_FAILURE);
            case OACK:
                handle_oack_packet(packet);
                break;
            default:
                error_exit("Invalid opcode.");
        }
        display_message(sock_fd, server_address, packet);
        while(true) {
            memset(packet, 0, options[BLKSIZE].value + 4);
            last = send_data_packet(sock_fd, server_address, ++out_block_number, file);
            memset(packet, 0, options[BLKSIZE].value + 4);
            if (recvfrom(sock_fd, (char *)packet, options[BLKSIZE].value + 4, MSG_WAITALL, (struct sockaddr *)&server_address, (socklen_t *)&server_address_size) < 0) {
                error_exit("Recvfrom failed on client side.");
            }
            opcode = opcode_get(packet);
            packet_pos = 0;
            switch (opcode) {
                case ACK:
                    handle_ack_packet(packet, out_block_number);
                    display_message(sock_fd, server_address, packet);
                    break;
                case ERROR:
                    display_message(sock_fd, server_address, packet);
                    fclose(file);
                    exit(EXIT_FAILURE);
                default:
                    error_exit("Invalid opcode.");
            }
            if (last == true) {
                fclose(file);
                break;
            }
        }
    }
    else {
        error_exit("Invalid request.");
    }
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);
    return EXIT_SUCCESS;
}

void init_args(ClientArgs_t *client_args) {
    client_args->host_name = calloc(MAX_STR_LEN, sizeof(char));
    client_args->port = DEFAULT_PORT_NUM;
    client_args->file_path = calloc(MAX_FILE_NAME_LEN, sizeof(char));
    client_args->dest_file_path = calloc(MAX_FILE_NAME_LEN, sizeof(char));
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

FILE *client_data_stream(int opcode, ClientArgs_t *client_args) {
    if (opcode == RRQ) {
        if (access(client_args->dest_file_path, F_OK) != -1) {
            error_exit("File already exists.");
        }
        file = fopen(client_args->dest_file_path, "w");
        if (file == NULL) {
            error_exit("Failed to open file.");
        }
    }
    else {
        file = stdin;
    }
    return file;
}