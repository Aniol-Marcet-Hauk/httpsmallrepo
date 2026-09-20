#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <getopt.h>
#include <zlib.h>
#include "circularbuffer.h"
#include "httprequestmap.h"
#include "httpresponsemap.h"


#define PORT "4221"   // Port we're listening on
#define BACKLOG 10
#define BUFFERSIZE 8192
#define TEMPBUFFSIZE 1024
#define HTTPNOTFOUND "HTTP/1.1 404 Not Found\r\n"
#define HTTPOK "HTTP/1.1 200 OK\r\n"
#define HTTPCREATED "HTTP/1.1 201 Created\r\n"

#define NUMENCOPTIONS 1
#define MAXSIZEOFENCODINGOPT 64
typedef struct httpheaderrequest{
    char * header;
    int headerlen;
    int headerspace;
    CircularBuffer *partialunprocessed;
    int * subheaderpos;
    int subhplen;
    int subhpspace;
    
}httpheaderrequest;

int min(int a, int b){
    int ret = a<b ? a: b;
    return ret;
}

void add_subheader(httpheaderrequest* request,int subh){
    request->subhplen++;
    if(request->subhplen>request->subhpspace){
        request->subhpspace +=5;
        request->subheaderpos = realloc(request->subheaderpos,sizeof(int)*request->subhpspace);
    }
    request->subheaderpos[request->subhplen-1] = subh;
}

int append_header_byte(httpheaderrequest* request, char value){
    if(request->headerlen >= request->headerspace){
        return 0;
    }
    request->header[request->headerlen] = value;
    request->headerlen++;
    return 1;
}

char * header_construct_string(httpheaderrequest* request, int from, int size, char * ret){
    if(from < 0 || size <= 0 || from + size > request->headerlen){
        return NULL;
    }
    memcpy(ret, request->header + from, size);
    ret[size] = '\0';
    return ret;
}

int header_get_from_till_delimstr_size(httpheaderrequest* request, int at, const char * delimstr, int delimlen){
    if(at < 0 || at >= request->headerlen){
        return -1;
    }
    for(int i = 0; at + i + delimlen <= request->headerlen; ++i){
        if(memcmp(request->header + at + i, delimstr, delimlen) == 0){
            return i;
        }
    }
    return -1;
}

int directoryflag = 0;
char * dirarg = NULL;
char* encodingoptions[NUMENCOPTIONS] ={"gzip"};



int gzipcompressionstring(const char * in, size_t inlen, char * out, size_t outlen, size_t *compressed_length){
    int ret;
    z_stream strm;

    if (compressed_length == NULL) {
        return -1;
    }
   
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *)in;
    strm.avail_in= (uLong) inlen;  
    strm.next_out = (Bytef *) out;
    strm.avail_out =(uLong)outlen;

    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8,   Z_DEFAULT_STRATEGY);
    if(ret != Z_OK){
        printf("error deflateInit2\n");
        return -1;
    }
    ret = deflate(&strm, Z_FINISH);
    (void)deflateEnd(&strm);
    if (ret != Z_STREAM_END) {
        return -1;
    }

    *compressed_length = strm.total_out;
    return 0;

}

void * storage_to_addr(struct sockaddr* socka){
    if(socka->sa_family == AF_INET){
        return  &(((struct sockaddr_in*)socka)->sin_addr);
    }
    else{
        return  &(((struct sockaddr_in6*)socka)->sin6_addr);
    }
}
int listener_socket(){
    struct addrinfo hints, *res, *p;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;
    int status;
    int sockfd;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(1);
    }
    int yes=1;
    for(p=res;p != NULL;p = p->ai_next){
        if((sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) < 0){
            perror("socket listener");
        
            continue;
        }
        

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) <0 ){
            perror("setsockopt");
            exit(1);
        }

        if(bind(sockfd,p->ai_addr,p->ai_addrlen) <0){
            perror("bind");
            close(sockfd);
            continue;
        }
        break;
    }
    
    if(p == NULL){
        perror("p == NULL :");
        exit(1);
    }
    if(listen(sockfd,BACKLOG)<0){
        perror("listen");
        exit(1);
    }
    freeaddrinfo(res);
    return sockfd;
}


void send_message(int fd, const char * main_header, const char * restheaders, const char * body, int body_size ){
    char headers[BUFFERSIZE];
    //right now it is basically impossible for restheaders to be BUFFERSIZE, But in the case that it is this must be changed
    int headlen = snprintf(headers,BUFFERSIZE, "%s%s\r\n",main_header,restheaders);
    if( send(fd,headers,headlen,0)< 0){
        perror("send");
    }
    if(body_size <= 0){
        return;
    }
    if( send(fd,body,body_size,0)< 0){
        perror("send");
    }
}

const char * connection_header_value(int connectionclose){
    return connectionclose ? "Connection: close\r\n" : "";
}

void set_basic_response(HttpResponseMap * response, const char * status, int connectionclose, const char * contenttype, const char * contentencoding){
    http_response_map_set_status(response, status);
    http_response_map_set_connection_close(response, connectionclose);
    http_response_map_set_content_type(response, contenttype);
    http_response_map_set_content_encoding(response, contentencoding);
}

int send_response_map(int fd, HttpResponseMap * response){
    char headers[BUFFERSIZE];
    char content_type_header[160] = "";
    char content_encoding_header[96] = "";
    const char * connection_header = connection_header_value(response->connection_close);
    const char * body = response->body;
    size_t body_length = response->body_length;
    char *encoded_body = NULL;

    if(response->status_line[0] == '\0'){
        return -1;
    }

    if(response->content_type[0] != '\0'){
        snprintf(content_type_header,sizeof(content_type_header),"Content-Type: %s\r\n",response->content_type);
    }

    if(response->content_encoding[0] != '\0'){
        snprintf(content_encoding_header,sizeof(content_encoding_header),"Content-Encoding: %s\r\n",response->content_encoding);
        if(strncmp(response->content_encoding,"gzip",4) == 0 && body != NULL && body_length > 0){
            size_t compressed_length = 0;
            size_t encoded_capacity = compressBound(body_length);
            encoded_body = malloc(encoded_capacity);
            if(encoded_body == NULL){
                return -1;
            }
            if (gzipcompressionstring(body, body_length, encoded_body, encoded_capacity, &compressed_length) < 0) {
                free(encoded_body);
                return -1;
            }
            body_length = compressed_length;
            body = encoded_body;
        }
    }

    int headlen = snprintf(headers,BUFFERSIZE,"%s%s%s%sContent-Length: %zu\r\n\r\n",
                response->status_line,
                connection_header,
                content_encoding_header,
                content_type_header,
                body_length);
    if( send(fd,headers,headlen,0)< 0){
        perror("send");
        free(encoded_body);
        return -1;
    }

    if(body_length > 0 && body != NULL){
        if( send(fd,body,body_length,0)< 0){
            perror("send");
            free(encoded_body);
            return -1;
        }
    }
    free(encoded_body);
    return 0;
}

void set_404_response(HttpResponseMap * response, int connectionclose){
    set_basic_response(response, HTTPNOTFOUND, connectionclose, NULL, NULL);
    http_response_map_set_body(response, NULL, 0);
}

int send_file(HttpRequestMap * request, HttpResponseMap * response, const char* contentencoding){
    char * filedir = request->path + 7;
    char fulldir[1024];
    size_t bytes_read;
    snprintf(fulldir,sizeof(fulldir),"%s/%s",dirarg,filedir);
    FILE * file = fopen(fulldir,"rb");
    if(file == NULL){
        perror("fopen");
        set_404_response(response,request->connection_close);
    }
    else{
        
        if (fseek(file, 0, SEEK_END) != 0) {
            fclose(file);
            return -1;
        }
        long fsize = ftell(file);
        if (fsize < 0) {
            fclose(file);
            return -1;
        }
        if (fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            return -1;
        }

        
        char *file_content = malloc(fsize + 1);
        if (file_content == NULL) {
            fclose(file);
            return -1;
        }
        //not quite sure how smart it is to read the entire thing at once?...
        bytes_read = fread(file_content, 1, (size_t)fsize, file);
        if (bytes_read != (size_t)fsize) {
            free(file_content);
            fclose(file);
            return -1;
        }
        file_content[fsize] = '\0';
        fclose(file);
        set_basic_response(response, HTTPOK, request->connection_close, "application/octet-stream", contentencoding);
        if(http_response_map_set_body(response, file_content, fsize) < 0){
            free(file_content);
            return -1;
        }
        free(file_content);
    }
    return 0;
}

int send_useragent(HttpRequestMap * request, HttpResponseMap * response, const char* contentencoding){
    set_basic_response(response, HTTPOK, request->connection_close, "text/plain", contentencoding);
    return http_response_map_set_body(response, request->user_agent, strlen(request->user_agent));
}

int handle_get_root(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption){
    (void)encodingoption;
    set_basic_response(response, HTTPOK, request->connection_close, NULL, NULL);
    return http_response_map_set_body(response, NULL, 0);
}

int handle_get_user_agent(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption){
   return send_useragent(request,response,encodingoption);
}

int handle_get_echo(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption){
    char *echostr = request->path+6;
    set_basic_response(response, HTTPOK, request->connection_close, "text/plain", encodingoption);
    return http_response_map_set_body(response, echostr, strlen(echostr));
}

int handle_get_file(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption){
   return send_file(request,response,encodingoption);
}

int POST_method(HttpRequestMap * request, HttpResponseMap * response, const char * encodingoption){
    (void)encodingoption;

    if(!directoryflag || strncmp(request->path,"/files/",7) != 0){
        set_404_response(response,request->connection_close); //provbably change
        return 0;
    }
 
    
    if(request->body_length == 0){
        set_404_response(response,request->connection_close);
        return 0;
    }

   
    
    char* filedir = request->path+7;
    char fulldir[1024];
    snprintf(fulldir,sizeof(fulldir),"%s/%s",dirarg,filedir);
    FILE *file = fopen(fulldir,"wb");
    if(file == NULL){
        perror("fopen");
        set_404_response(response,request->connection_close);
        return 0;
    }

    if(fwrite(request->body,1,request->body_length,file) <= 0) {
        perror("fwrite");
        fclose(file);
        return -1;
        //should probably also check if partial write
    }  
    fclose(file);
    set_basic_response(response, HTTPCREATED, request->connection_close, NULL, NULL);
    return http_response_map_set_body(response, NULL, 0);
}

typedef int (*RouteHandler)(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption);

typedef enum RouteMatchType {
    ROUTE_MATCH_EXACT,
    ROUTE_MATCH_PREFIX
} RouteMatchType;

typedef struct RouteDefinition {
    const char *method;
    const char *pattern;
    RouteMatchType match_type;
    int require_directory;
    RouteHandler handler;
} RouteDefinition;

int route_matches(RouteDefinition route, HttpRequestMap * request){
    if(strcmp(request->method, route.method) != 0){
        return 0;
    }
    if(route.require_directory && !directoryflag){
        return 0;
    }
    if(route.match_type == ROUTE_MATCH_EXACT){
        return strcmp(request->path, route.pattern) == 0;
    }
    return strncmp(request->path, route.pattern, strlen(route.pattern)) == 0;
}

int dispatch_route(HttpRequestMap * request, HttpResponseMap * response, const char* encodingoption){
    static RouteDefinition routes[] = {
        {"GET", "/", ROUTE_MATCH_EXACT, 0, handle_get_root},
        {"GET", "/user-agent", ROUTE_MATCH_EXACT, 0, handle_get_user_agent},
        {"GET", "/echo/", ROUTE_MATCH_PREFIX, 0, handle_get_echo},
        {"GET", "/files/", ROUTE_MATCH_PREFIX, 1, handle_get_file},
        {"POST", "/files/", ROUTE_MATCH_PREFIX, 1, POST_method}
    };
    size_t route_count = sizeof(routes) / sizeof(routes[0]);

    for(size_t i = 0; i < route_count; ++i){
        if(route_matches(routes[i], request)){
            return routes[i].handler(request, response, encodingoption);
        }
    }

    set_404_response(response,request->connection_close);
    return 0;
}
typedef struct threadargs{
    int fd;
}targs;

char * get_accepted_encoding(char * opt ){
    if(opt == NULL || opt[0] == '\0'){
        return NULL;
    }
    char * saveptr;
    char * token = strtok_r(opt,", ", &saveptr);
    while(token!= NULL){
        for(size_t i= 0; i<NUMENCOPTIONS;++i){
            if(strcasecmp(token,encodingoptions[i]) == 0){
                return encodingoptions[i];
            }
        }
        token = strtok_r(NULL,", ;", &saveptr);//;for the quality score, yes i know i'm not taking that into account right now
    }

    return NULL;
}

void clear_request(httpheaderrequest* request){
    request->headerlen = 0;
    request->subhplen = 0;
}

int append_header_chunk(httpheaderrequest* request, const char * chunk, int chunklen, int * hfelements, int * fullrequest, int * bytesprocessed, int totalbytes){
    char headerfinishedcheck[2] = {'\r','\n'};

    for(int i = 0; i < chunklen && !(*fullrequest); ++i){
        if(chunk[i] == headerfinishedcheck[(*hfelements % 2)]){
            (*hfelements)++;
            if(*hfelements == 2){
                add_subheader(request,totalbytes+i+1);
            }
            if(*hfelements == 4){
                *fullrequest = 1;
                *bytesprocessed = i+1;
            }
        }
        else{
            *hfelements = 0;
        }

        if(!append_header_byte(request,chunk[i])){
            printf("Header to big\n");
            return -1;
        }
    }

    return 0;
}

int process_header_request(int thisfd,httpheaderrequest* request){

    //Probably should restructure this function

    clear_request(request);
    int bytesrecv = 0;
    int totalbytes = 0;
    int fullrequest = 0;
    char temp[TEMPBUFFSIZE];
    int hfelements = 0;
    int bytesprocessed = -1;
    add_subheader(request,0);
    if(request->partialunprocessed->filled>0){
        bytesrecv = request->partialunprocessed->filled;
        char preloaded[bytesrecv];
        for(int i =0; i<bytesrecv && !fullrequest;++i){
            preloaded[i] = buffer_pop(request->partialunprocessed);
        }
        if(append_header_chunk(request, preloaded, bytesrecv, &hfelements, &fullrequest, &bytesprocessed, totalbytes) < 0){
            return -1;
        }
        if(fullrequest)
            totalbytes+=bytesprocessed;
        else
            totalbytes+= bytesrecv;
        //STOPED WHILE IMPLEMENTING ALL THE PARSING, IN THE MIDDLE OF THE NEXTHEADER PARSE
    }

    while(!fullrequest){
        if((bytesrecv = recv(thisfd,temp,TEMPBUFFSIZE-1,0))<0){
            perror("recv");
            close(thisfd);
            return -1;
        }
        if(bytesrecv == 0 ){
            return 0;
        }

        if(append_header_chunk(request, temp, bytesrecv, &hfelements, &fullrequest, &bytesprocessed, totalbytes) < 0){
            return -1;
        }
        if(fullrequest)
            totalbytes+=bytesprocessed;
        else
            totalbytes+= bytesrecv;
        
    }
    if(bytesprocessed!= bytesrecv){
        for(int i =bytesprocessed;i<bytesrecv;++i){
            if(buffer_free_el(request->partialunprocessed))
                buffer_push(request->partialunprocessed,temp[i]);
            else{
                printf("circularbuffer full, some kind of error\n");
            }
        }   
    }
    
    return totalbytes;

    
}
//body should be initialized
int process_body_request(int thisfd, httpheaderrequest* request, char * body, int len){
    if(len <= 0){
        body = NULL;
        return 0;
    }
    int bodyinunprocessed = min(request->partialunprocessed->filled, len);
    for(int i= 0; i<bodyinunprocessed;++i){
        body[i] = buffer_pop(request->partialunprocessed);
    }
    if(bodyinunprocessed==len){
        return bodyinunprocessed;
    }
    //case body is not fully in unprocessed ( this basically means that unprocessed will be empty for the next http read)
    int bodyprocessed = bodyinunprocessed;
    char temp[TEMPBUFFSIZE];
    int bytesrecv = 0;
    while(bodyprocessed< len){
        int maxread = min(TEMPBUFFSIZE,len-bodyprocessed);
        if((bytesrecv = recv(thisfd,temp,maxread,0))<0){
            perror("recv");
            close(thisfd);
            return -1;
        }
        if(bytesrecv == 0){
            return 0;
        }
        for(int i =0;i<bytesrecv; ++i){
            body[bodyprocessed+i] = temp[i];
        }
        bodyprocessed+=bytesrecv;
        
    }
    if(bodyprocessed>len){
        printf("read more bytes?\n");//this is just a check incase my code is wrong
        return -1;
    }
    return bodyprocessed; //should be equal to len
}
void * threadTCP(void * args){
    
    targs * targ = (targs*) args;
    int thisfd = targ->fd;
    free(targ);
    pthread_detach(pthread_self());
    httpheaderrequest request;
    HttpRequestMap requestmap;
    HttpResponseMap responsemap;

    request.header = malloc(BUFFERSIZE);
    request.headerlen = 0;
    request.headerspace = BUFFERSIZE;
    request.partialunprocessed= init_buffer(BUFFERSIZE);
    request.subhpspace =5;
    request.subheaderpos = malloc(sizeof(int)*request.subhpspace);
    if(request.header == NULL || request.partialunprocessed == NULL || request.subheaderpos == NULL){
        free(request.header);
        if(request.partialunprocessed != NULL){
            free_buffer(request.partialunprocessed);
        }
        free(request.subheaderpos);
        close(thisfd);
        return NULL;
    }
    http_request_map_init(&requestmap);
    http_response_map_init(&responsemap);
    while(1){
      
        
        if(process_header_request(thisfd,&request)<=0){
            break;
        }
        http_request_map_reset(&requestmap);
        http_response_map_reset(&responsemap);

        int requestlinesize = header_get_from_till_delimstr_size(&request,0,"\r\n",2);
        if(requestlinesize < 0){
            break;
        }
        char requestline[requestlinesize+1];
        if(header_construct_string(&request,0,requestlinesize,requestline) == NULL){
            break;
        }
        http_request_map_apply_request_line(&requestmap, requestline);

        int posinline = 0;
        for(int i = 1; i<request.subhplen-1; ++i){
            posinline = request.subheaderpos[i];
            if(posinline >= request.headerlen)
                break;
            int headerlinesize = header_get_from_till_delimstr_size(&request,posinline,"\r\n",2);
            if(headerlinesize < 0){
                continue;
            }
            char headerline[headerlinesize+1];
            if(header_construct_string(&request,posinline,headerlinesize,headerline) == NULL){
                continue;
            }
            http_request_map_apply_header(&requestmap, headerline);
        }
        if(requestmap.method[0] == '\0' || requestmap.path[0] == '\0'){
            break;
        }

        char * encodingoption = get_accepted_encoding(requestmap.accept_encoding);
        int contentlen = requestmap.content_length;
        if(contentlen < 0){
            break;
        }

        char content[contentlen+1];
        if(process_body_request(thisfd,&request,content,contentlen) != contentlen){
            break;
        }
        content[contentlen] = '\0';
        if(http_request_map_set_body(&requestmap, content, contentlen) < 0){
            break;
        }
        if(dispatch_route(&requestmap,&responsemap,encodingoption) < 0){
            break;
        }

        if(send_response_map(thisfd,&responsemap) < 0){
            break;
        }

        if(requestmap.connection_close){
            break;
        }
        
    } 
    
    http_request_map_free(&requestmap);
    http_response_map_free(&responsemap);
    free(request.header);
    free_buffer(request.partialunprocessed);
    free(request.subheaderpos);
    
    close(thisfd);
    return NULL;
   
   
}
int main(int argc, char* argv[]){
    int sockfd = listener_socket();
    struct sockaddr_storage insock;
    socklen_t inlen = sizeof(insock);
    int newfd;
    int opt;
    static struct option options[] ={
        {"directory",required_argument,0,'d'},
        {0,0,0,0}
    };

   
    while((opt = getopt_long(argc,argv,"d:",options,NULL))!=-1){
        switch (opt)
        {
        case 'd':
            directoryflag =1;
            dirarg = optarg;
            break;
        default:
            break;
        }
    }
    
    while(1){
        if((newfd = accept(sockfd,(struct sockaddr*)&insock,&inlen)) <0){
            perror("accept");
            continue;
        }
        targs* passval = malloc(sizeof(targs));
        passval->fd = newfd;
      
        pthread_t thread;
        pthread_create(&thread,NULL,threadTCP,passval);
        

    }
   
    close(sockfd);
    
 
    return 0;
}