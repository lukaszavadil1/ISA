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
    ClientArgs_t client_args;
    parse_args(argc, argv, &client_args);
    return 0;
}

void parse_args(int argc, char *argv[], ClientArgs_t *client_args) {
    if (argc > 9 || argc < 5) { 
        error_exit("Invalid number of arguments.");
    }
    int opt;
    bool h_flag = false, p_flag = false, f_flag = false, t_flag = false;
    while ((opt = getopt(argc, argv, ":h:p:f:t:")) != -1) {
        switch (opt) {
            case 'h':
                if (h_flag) {
                    error_exit("Duplicate option.");
                }
                h_flag = true;
                break;
            case 'p':
                if (p_flag) {
                    error_exit("Duplicate option.");
                }
                p_flag = true;
                break;
            case 'f':
                if (f_flag) {
                    error_exit("Duplicate option.");
                }
                f_flag = true;
                break;
            case 't':
                if (t_flag) {
                    error_exit("Duplicate option.");
                }
                t_flag = true;
                break;
            case '?':
                error_exit("Unknown option.");
                break;
            case ':':
                error_exit("Missing argument.");
                break;
            default:
                error_exit("Unknown error.");
                break;
        }
        if (argv[optind] != NULL) {
            if ((strcmp(argv[optind], "-h") != 0) && (strcmp(argv[optind], "-p") != 0) && (strcmp(argv[optind], "-f") != 0) && (strcmp(argv[optind], "-t") != 0)) {
                error_exit("Flag must have only one argument.");
            }
        }
    }
    if (h_flag == false || t_flag == false) {
        error_exit("Missing mandatory argument.");
    }
}