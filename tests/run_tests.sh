#!/usr/bin/env sh
set -eu

mkdir -p tests/bin

gcc -Wall -Wextra -pthread -I. tests/http_test_client.c tests/test_basic.c httprequestmap.c httpresponsemap.c -o tests/bin/test_basic
gcc -Wall -Wextra -pthread -I. tests/http_test_client.c tests/test_files.c httprequestmap.c httpresponsemap.c -o tests/bin/test_files
gcc -Wall -Wextra -pthread -I. tests/http_test_client.c tests/test_concurrency.c httprequestmap.c httpresponsemap.c -o tests/bin/test_concurrency

tests/bin/test_basic
tests/bin/test_files
tests/bin/test_concurrency
