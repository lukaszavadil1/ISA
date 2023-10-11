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
    ServerArgs_t *server_args;
    server_args = malloc(sizeof(ServerArgs_t));
    init_args(server_args);
    parse_args(argc, argv, server_args);
    printf("Port: %d\n", server_args->port);
    printf("Directory path: %s\n", server_args->dir_path);
    free_args(server_args);
    return 0;
}

void init_args(ServerArgs_t *server_args) {
    server_args->port = 69;
    server_args->dir_path = malloc(MAX_STR_LEN);
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
                    error_exit("Duplicate option.");
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