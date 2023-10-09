//
// File: utils.c
//
// Author: Lukáš Zavadil
//
// Description: Implementation of helper functions.
//

#include "../include/utils.h"

void error_exit(const char *message) {
    if (errno == 0) {
        fprintf(stderr, "Error: %s\n", message);
    } else {
        fprintf(stderr, "Error: %s (%s)\n", message, strerror(errno));
    }
    exit(EXIT_FAILURE);
}