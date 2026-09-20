#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct{
    int size;
    int filled;
    char* items;
    int init;//inclusive
   
}CircularBuffer;


CircularBuffer* init_buffer(int size) {

    CircularBuffer* buff = malloc(sizeof(CircularBuffer)); 
    if (buff == NULL) return NULL;

    buff->items = malloc(size);
    if (buff->items == NULL) {
        free(buff);
        return NULL;
    }
    buff->size = size;
    buff->filled = 0;
    buff->init = 0;

    return buff;
}
int get_index_buff(CircularBuffer * buff,int at){
    if(at < 0 || at>=buff->filled){
        return -1;
    }
    return ((buff->init +at)%buff->size);
}
char get_char_at_buff(CircularBuffer * buff,int at){
    int index = get_index_buff(buff,at);
    if(index < 0){
        return '\0';
    }
    return buff->items[index];
}
//ret -1 if filled is > size, 0 if full and 1 else
int buffer_push( CircularBuffer * buff, char item){
    
    if(buff->filled +1>buff->size){
        return -1;
    }
    buff->filled++;
    buff->items[get_index_buff(buff,buff->filled-1)] = item;
 
    if(buff->filled == buff->size){
        return 0;
    }
    return 1;
}
//always check that buff is not empty before calling
char buffer_pop(CircularBuffer * buff){
    if(buff->filled <=0){
        printf("tried to pop empty circbuff");
        return '\0'; //i dont know...
    }
    int index = get_index_buff(buff,0);
    char ret = buff->items[index];
    buff->items[index] = '\0';
    buff->init = (buff->init+1)%buff->size;
    buff->filled--;
    return ret;
}
//if eof returns pos of last element if no delim
//else returns the num of element till the first delim ( delim not included), or -1 if there is nothing
int elements_till_next_delimiter(CircularBuffer * buff, char * delimiters, int eof){
    
    int index;
    for(int i = 0; i<buff->filled;++i){
        index = get_index_buff(buff,i);
        for(size_t c = 0; c<strlen(delimiters);++c){
            if(buff->items[index] == delimiters[c]){
                return i;
            }
        }
    }
    if(eof)
        return buff->filled;
    return -1;
}

int buffer_free_el(CircularBuffer * buff){
    return (buff->size-buff->filled);
}
int buffer_strncomp_at(CircularBuffer * buff, int at, char* strcomp, int n){
    if(at+n>buff->filled|| at>= buff->filled){
        printf("error strncomp");
        return -1;
    }
    int index;
    for(int i = 0; i<n;++i){
        index = get_index_buff(buff,at+i);
        char c = buff->items[index];
        char c2 = strcomp[i];
        if(c!=c2){
            return c-c2;
        }
    }
    return 0;
    
}
int buffer_strcasencomp_at(CircularBuffer * buff, int at, char* strcomp, int n){
    if(at>= buff->filled){
        printf("error strncomp case");
        return -1;
    }
    if(at+n>buff->filled){
        return -1;//strcomp is bigger than etc
    }
    int index;
    for(int i = 0; i<n;++i){
        index = get_index_buff(buff,at+i);
        char c = tolower(buff->items[index]);
        char c2 = tolower(strcomp[i]);
        if(c!=c2){
            return c-c2;
        }
    }
    return 0;
    
}
char * buffer_construct_string(CircularBuffer * buff, int from, int size, char * ret){
     if(from+size>buff->filled|| from>= buff->filled || size == 0){
        return NULL;
     }
     int index;
     
     for(int i = 0; i<size;++i){
        index = get_index_buff(buff,from+i);
        ret[i] = buff->items[index];
     }
     ret[size] = '\0';
     return ret;
}
int buffer_get_from_till_delimstr_size(CircularBuffer * buff, int at, char * delimstr,int delimlen){
    if(at>= buff->filled){
        return -1;
    }
    for(int i = 0; at+i< buff->filled; ++i){
        
        if(buffer_strncomp_at(buff,at+i,delimstr,delimlen) == 0){
            return i;
        }
    }

    return 0;

}

int buffer_get_from_till_delimstr_atoint(CircularBuffer * buff, int at, char * delimstr,int delimlen){
    if(at>= buff->filled){
        return -1;
    }
    int ret = 0;
    
    for(int i = 0; at+i< buff->filled; ++i){
        
        if(buffer_strncomp_at(buff,at+i,delimstr,delimlen) == 0){
           return ret;
            
        }
        if(buff->items[get_index_buff(buff,at+i)] == ' '){
            continue;
        }
        
        ret*=10;
        ret+= (int)(buff->items[get_index_buff(buff,at+i)] -'0');
    }

    return -1;

}


void free_buffer(CircularBuffer* buff){
    free(buff->items);
    free(buff);
}


void free_allitems_buffer(CircularBuffer * buff){
     if(buff->filled ==0){
        return;
    }
    buff->init = 0;
    buff->filled = 0;

    return;
}






