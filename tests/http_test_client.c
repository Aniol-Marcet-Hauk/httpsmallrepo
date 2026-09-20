#include "http_test_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_expect_true(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "test failed: %s\n", message);
        exit(1);
    }
}

void test_expect_int_eq(int actual, int expected, const char *message) {
    if (actual != expected) {
        fprintf(stderr, "test failed: %s (got %d, expected %d)\n", message, actual, expected);
        exit(1);
    }
}

void test_expect_str_eq(const char *actual, const char *expected, const char *message) {
    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "test failed: %s (got '%s', expected '%s')\n", message, actual, expected);
        exit(1);
    }
}