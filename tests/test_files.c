#include "http_test_client.h"

#include "../httpresponsemap.h"

static void test_response_map_body_and_headers(void) {
    HttpResponseMap response;

    http_response_map_init(&response);
    http_response_map_set_status(&response, "HTTP/1.1 200 OK");
    http_response_map_set_content_type(&response, "text/plain");
    http_response_map_set_content_encoding(&response, "gzip");
    http_response_map_set_connection_close(&response, 1);

    test_expect_str_eq(response.status_line, "HTTP/1.1 200 OK", "status line should be stored");
    test_expect_str_eq(response.content_type, "text/plain", "content type should be stored");
    test_expect_str_eq(response.content_encoding, "gzip", "content encoding should be stored");
    test_expect_int_eq(response.connection_close, 1, "connection close should be stored");

    test_expect_int_eq(http_response_map_set_body(&response, "payload", 7), 0, "body should be stored");
    test_expect_str_eq(response.body, "payload", "response body should be copied");
    test_expect_true(response.body_length == 7, "response body length should be stored");

    http_response_map_reset(&response);
    test_expect_true(response.status_line[0] == '\0', "reset should clear status");
    test_expect_true(response.body == NULL, "reset should release response body");
    test_expect_true(response.body_length == 0, "reset should clear response body length");
}

int main(void) {
    test_response_map_body_and_headers();
    return 0;
}