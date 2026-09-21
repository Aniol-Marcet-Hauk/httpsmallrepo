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
