#include "httprequestmap.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void http_request_map_clear_body(HttpRequestMap *request) {
    free(request->body);
    request->body = NULL;
    request->body_length = 0;
}

static void http_request_map_copy_segment(char *dest, size_t dest_size, const char *start, size_t len) {
    size_t offset = 0;
    size_t end = len;

    if (dest_size == 0) {
        return;
    }

    while (offset < len && isspace((unsigned char)start[offset])) {
        offset++;
    }
    while (end > offset && isspace((unsigned char)start[end - 1])) {
        end--;
    }

    len = end - offset;
    if (len >= dest_size) {
        len = dest_size - 1;
    }

    memcpy(dest, start + offset, len);
    dest[len] = '\0';
}

static int http_request_map_has_token(const char *value, const char *token) {
    const char *cursor = value;

    while (*cursor != '\0') {
        const char *token_end = cursor;
        while (*token_end != '\0' && *token_end != ',') {
            token_end++;
        }

        while (cursor < token_end && isspace((unsigned char)*cursor)) {
            cursor++;
        }
        while (token_end > cursor && isspace((unsigned char)token_end[-1])) {
            token_end--;
        }

        if ((size_t)(token_end - cursor) == strlen(token) && strncasecmp(cursor, token, strlen(token)) == 0) {
            return 1;
        }

        cursor = *token_end == '\0' ? token_end : token_end + 1;
    }

    return 0;
}

void http_request_map_init(HttpRequestMap *request) {
    memset(request, 0, sizeof(*request));
}

void http_request_map_reset(HttpRequestMap *request) {
    http_request_map_clear_body(request);
    memset(request, 0, sizeof(*request));
}

void http_request_map_free(HttpRequestMap *request) {
    http_request_map_clear_body(request);
}

void http_request_map_apply_request_line(HttpRequestMap *request, const char *request_line) {
    const char *method_end = strchr(request_line, ' ');
    const char *path_start;
    const char *path_end;

    if (method_end == NULL) {
        return;
    }

    path_start = method_end + 1;
    while (*path_start == ' ') {
        path_start++;
    }

    path_end = strchr(path_start, ' ');
    if (path_end == NULL) {
        path_end = path_start + strlen(path_start);
    }

    http_request_map_copy_segment(request->method, sizeof(request->method), request_line, (size_t)(method_end - request_line));
    http_request_map_copy_segment(request->path, sizeof(request->path), path_start, (size_t)(path_end - path_start));
}

void http_request_map_apply_header(HttpRequestMap *request, const char *header_line) {
    const char *separator = strchr(header_line, ':');
    char header_name[64];
    const char *header_value;

    if (separator == NULL) {
        return;
    }

    http_request_map_copy_segment(header_name, sizeof(header_name), header_line, (size_t)(separator - header_line));
    header_value = separator + 1;

    if (strcasecmp(header_name, "Content-Type") == 0) {
        http_request_map_copy_segment(request->content_type, sizeof(request->content_type), header_value, strlen(header_value));
    }
    else if (strcasecmp(header_name, "Content-Length") == 0) {
        char content_length_text[32];
        http_request_map_copy_segment(content_length_text, sizeof(content_length_text), header_value, strlen(header_value));
        request->content_length = atoi(content_length_text);
    }
    else if (strcasecmp(header_name, "Accept-Encoding") == 0) {
        http_request_map_copy_segment(request->accept_encoding, sizeof(request->accept_encoding), header_value, strlen(header_value));
    }
    else if (strcasecmp(header_name, "User-Agent") == 0) {
        http_request_map_copy_segment(request->user_agent, sizeof(request->user_agent), header_value, strlen(header_value));
    }
    else if (strcasecmp(header_name, "Connection") == 0) {
        char connection_value[64];
        http_request_map_copy_segment(connection_value, sizeof(connection_value), header_value, strlen(header_value));
        if (http_request_map_has_token(connection_value, "close")) {
            request->connection_close = 1;
        }
    }
}

int http_request_map_set_body(HttpRequestMap *request, const char *body, int body_length) {
    http_request_map_clear_body(request);

    if (body_length <= 0) {
        return 0;
    }

    if (body == NULL) {
        return -1;
    }

    request->body = malloc((size_t)body_length + 1);
    if (request->body == NULL) {
        return -1;
    }

    memcpy(request->body, body, (size_t)body_length);
    request->body[body_length] = '\0';
    request->body_length = body_length;
    return 0;
}
