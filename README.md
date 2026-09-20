# httpsmallrepo

A small multithreaded HTTP server written in C.

## What this project is

This project is a lightweight HTTP server that listens on port `4221` and handles a small set of routes. It supports:

- `GET /`
- `GET /echo/<text>`
- `GET /user-agent`
- `GET /files/<name>`
- `POST /files/<name>`

It also supports persistent connections, `Connection: close`, basic file serving, request parsing, and optional gzip response compression when the client accepts it.

The code is split into smaller modules so the request parsing, request state, response state, and circular buffer logic are easier to work with.



## Build

Compile the server with:

```sh
gcc -Wall -Wextra -pthread http.c circularbuffer.c httprequestmap.c httpresponsemap.c -lz -o httpserver
```

## Run

Start the server with:

```sh
./httpserver --directory temp
```

The `--directory` argument enables file routes and points them at the directory you want to serve.

## Tests



Run tests with

```sh
bash ./tests/run_tests.sh
```


## Notes

- The server listens on `4221`.
- Requests are handled in separate threads.
- The tests expect the server to already be running before you execute the test runner
