#pragma once

#include <stddef.h>

#define HTTPREQUESTMAP_METHOD_SIZE 16
#define HTTPREQUESTMAP_PATH_SIZE 1024
#define HTTPREQUESTMAP_USER_AGENT_SIZE 512
#define HTTPREQUESTMAP_ENCODING_SIZE 128
#define HTTPREQUESTMAP_CONTENT_TYPE_SIZE 128

typedef struct HttpRequestMap {
    char method[HTTPREQUESTMAP_METHOD_SIZE];
    char path[HTTPREQUESTMAP_PATH_SIZE];
    char user_agent[HTTPREQUESTMAP_USER_AGENT_SIZE];
    char accept_encoding[HTTPREQUESTMAP_ENCODING_SIZE];
    char content_type[HTTPREQUESTMAP_CONTENT_TYPE_SIZE];
    int content_length;
    int connection_close;
    char *body;
    int body_length;
} HttpRequestMap;

void http_request_map_init(HttpRequestMap *request);
void http_request_map_reset(HttpRequestMap *request);
void http_request_map_free(HttpRequestMap *request);
void http_request_map_apply_request_line(HttpRequestMap *request, const char *request_line);
void http_request_map_apply_header(HttpRequestMap *request, const char *header_line);
int http_request_map_set_body(HttpRequestMap *request, const char *body, int body_length);
