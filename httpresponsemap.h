#pragma once

#include <stddef.h>

#define HTTPRESPONSEMAP_STATUS_SIZE 64
#define HTTPRESPONSEMAP_CONTENT_TYPE_SIZE 128

#define HTTPRESPONSEMAP_CONTENT_ENCODING_SIZE 64

typedef struct HttpResponseMap {
    char status_line[HTTPRESPONSEMAP_STATUS_SIZE];
    char content_type[HTTPRESPONSEMAP_CONTENT_TYPE_SIZE];
    char content_encoding[HTTPRESPONSEMAP_CONTENT_ENCODING_SIZE];


    int connection_close;
    char *body;
    size_t body_length;


} HttpResponseMap;



void http_response_map_init(HttpResponseMap *response);

void http_response_map_reset(HttpResponseMap *response);
void http_response_map_free(HttpResponseMap *response);



int http_response_map_set_body(HttpResponseMap *response, const char *body, size_t body_length);
