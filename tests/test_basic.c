#include "http_test_client.h"

#include "../httprequestmap.h"

static void test_request_map_parsing(void) {
    HttpRequestMap request;

    http_request_map_init(&request);
    http_request_map_apply_request_line(&request, "POST   /submit-form  HTTP/1.1");
    http_request_map_apply_header(&request, "Content-Type: text/plain");
    http_request_map_apply_header(&request, "Content-Length: 5");
    http_request_map_apply_header(&request, "Accept-Encoding: gzip, deflate");
    http_request_map_apply_header(&request, "User-Agent: Test Client");
    http_request_map_apply_header(&request, "Connection: keep-alive, close");

    test_expect_str_eq(request.method, "POST", "request method should be parsed");
    test_expect_str_eq(request.path, "/submit-form", "request path should be parsed");
    test_expect_str_eq(request.content_type, "text/plain", "content type should be stored");
    test_expect_int_eq(request.content_length, 5, "content length should be stored");
    test_expect_str_eq(request.accept_encoding, "gzip, deflate", "accept encoding should be stored");
    test_expect_str_eq(request.user_agent, "Test Client", "user agent should be stored");
    test_expect_int_eq(request.connection_close, 1, "connection close should detect close token");

    test_expect_int_eq(http_request_map_set_body(&request, "hello", 5), 0, "body should be stored");
    test_expect_str_eq(request.body, "hello", "body content should be copied");
    test_expect_int_eq(request.body_length, 5, "body length should be stored");

    http_request_map_reset(&request);
    test_expect_true(request.method[0] == '\0', "reset should clear method");
    test_expect_true(request.path[0] == '\0', "reset should clear path");
    test_expect_true(request.body == NULL, "reset should release body");
    test_expect_int_eq(request.body_length, 0, "reset should clear body length");
    test_expect_int_eq(request.content_length, 0, "reset should clear content length");
}

int main(void) {
    test_request_map_parsing();
    return 0;
}