#pragma once

typedef struct {
    int size;
    int filled;
    char *items;
    int init;
} CircularBuffer;

CircularBuffer * init_buffer(int size);
int get_index_buff(CircularBuffer * buff, int at);
char get_char_at_buff(CircularBuffer * buff, int at);
int buffer_push(CircularBuffer * buff, char item);
char buffer_pop(CircularBuffer * buff);
int elements_till_next_delimiter(CircularBuffer * buff, char * delimiters, int eof);
int buffer_free_el(CircularBuffer * buff);
int buffer_strncomp_at(CircularBuffer * buff, int at, char * strcomp, int n);
int buffer_strcasencomp_at(CircularBuffer * buff, int at, char * strcomp, int n);
char * buffer_construct_string(CircularBuffer * buff, int from, int size, char * ret);
int buffer_get_from_till_delimstr_size(CircularBuffer * buff, int at, char * delimstr, int delimlen);
int buffer_get_from_till_delimstr_atoint(CircularBuffer * buff, int at, char * delimstr, int delimlen);
void free_buffer(CircularBuffer * buff);
void free_allitems_buffer(CircularBuffer * buff);
