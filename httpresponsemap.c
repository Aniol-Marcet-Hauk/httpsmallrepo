#include "httpresponsemap.h"

#include <stdlib.h>
#include <string.h>

static void http_response_map_clear_body(HttpResponseMap *response) {
    free(response->body);
    response->body = NULL;
    response->body_length = 0;
}

static void http_response_map_copy_string(char *dest, size_t dest_size, const char *src) {
    size_t len;

    if (dest_size == 0) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    len = strlen(src);
    if (len >= dest_size) {
        len = dest_size - 1;
    }

    memcpy(dest, src, len);
    dest[len] = '\0';
}

void http_response_map_init(HttpResponseMap *response) {
    memset(response, 0, sizeof(*response));
}

void http_response_map_reset(HttpResponseMap *response) {
    http_response_map_clear_body(response);
    memset(response, 0, sizeof(*response));
}

void http_response_map_free(HttpResponseMap *response) {
    http_response_map_clear_body(response);
}

void http_response_map_set_status(HttpResponseMap *response, const char *status_line) {
    http_response_map_copy_string(response->status_line, sizeof(response->status_line), status_line);
}

void http_response_map_set_content_type(HttpResponseMap *response, const char *content_type) {
    http_response_map_copy_string(response->content_type, sizeof(response->content_type), content_type);
}

void http_response_map_set_content_encoding(HttpResponseMap *response, const char *content_encoding) {
    http_response_map_copy_string(response->content_encoding, sizeof(response->content_encoding), content_encoding);
}

void http_response_map_set_connection_close(HttpResponseMap *response, int connection_close) {
    response->connection_close = connection_close;
}

int http_response_map_set_body(HttpResponseMap *response, const char *body, size_t body_length) {
    http_response_map_clear_body(response);

    if (body == NULL || body_length == 0) {
        return 0;
    }

    response->body = malloc(body_length + 1);
    if (response->body == NULL) {
        return -1;
    }

    memcpy(response->body, body, body_length);
    response->body[body_length] = '\0';
    response->body_length = body_length;
    return 0;
}
