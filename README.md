# httpfinal

A small multithreaded HTTP server written in C.


## Dependencies

Required:

- GCC or another C compiler compatible with the build command
- POSIX threads support (used via `-pthread`)º
- zlib development headers/library 

On Linux, install at least your C build toolchain and zlib dev package.

- Ubuntu: `sudo apt install build-essential zlib1g-dev`



## Overview

This project is a lightweight HTTP server that listens on port `4221` and handles a small set of routes. It supports:

- `GET /`
- `GET /echo/<text>`
- `GET /user-agent`
- `GET /files/<name>`
- `POST /files/<name>`

It also supports persistent connections, `Connection: close`, basic file serving, request parsing, and gzip compression

The code is split into smaller modules so the request parsing, request state, response state, and circular buffer logic are easier to work with.

## Motivation

I originally wanted to make a small OS but it turns out that small OS are actually kind of big... So i decided to do a http server. 

I used beej's guide https://beej.us/guide/bgnet/b 
and codecrafters http server in c course as a jumping off point.

## Build

Compile the server with:

```sh
gcc -Wall -Wextra -pthread http.c circularbuffer.c httprequestmap.c httpresponsemap.c -lz -o httpserver
```

## Run

Start the server with:

```sh
./httpserver 
```

If you want to enable file routes and point them to a directory you want:

```sh
./httpserver --directory temp
```

## Tests

Run the server first

Then run tests with

```sh
bash ./tests/run_tests.sh
```


Then run tests with

```sh
bash ./tests/run_tests.sh
```

