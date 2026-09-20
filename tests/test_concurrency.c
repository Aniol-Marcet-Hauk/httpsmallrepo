#include "http_test_client.h"

#include "../httprequestmap.h"
#include "../httpresponsemap.h"

#include <pthread.h>

enum { WORKER_COUNT = 4, WORKER_ITERATIONS = 1000 };

static void *worker_run(void *arg) {
    long worker_id = (long)arg;
    HttpRequestMap request;
    HttpResponseMap response;

    http_request_map_init(&request);
    http_response_map_init(&response);

    for (int i = 0; i < WORKER_ITERATIONS; ++i) {
        http_request_map_apply_request_line(&request, "POST /worker HTTP/1.1");
        http_request_map_apply_header(&request, "Content-Length: 4");
        test_expect_int_eq(http_request_map_set_body(&request, "body", 4), 0, "request body should be set in worker");

        http_response_map_set_status(&response, "HTTP/1.1 200 OK");
        http_response_map_set_content_type(&response, "text/plain");

        test_expect_true(request.method[0] == 'P', "worker should keep request data intact");
        test_expect_true(worker_id >= 0, "worker id should be non-negative");
    }

    http_request_map_free(&request);
    http_response_map_free(&response);
    return NULL;
}

int main(void) {
    pthread_t threads[WORKER_COUNT];

    for (long i = 0; i < WORKER_COUNT; ++i) {
        test_expect_int_eq(pthread_create(&threads[i], NULL, worker_run, (void *)i), 0, "thread creation should succeed");
    }

    for (int i = 0; i < WORKER_COUNT; ++i) {
        test_expect_int_eq(pthread_join(threads[i], NULL), 0, "thread join should succeed");
    }

    return 0;
}