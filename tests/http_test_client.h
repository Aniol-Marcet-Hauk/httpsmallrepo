#pragma once

void test_expect_true(int condition, const char *message);
void test_expect_int_eq(int actual, int expected, const char *message);
void test_expect_str_eq(const char *actual, const char *expected, const char *message);