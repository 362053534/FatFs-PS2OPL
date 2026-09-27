#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
bin=$(mktemp)
trap 'rm -f "$bin"' EXIT HUP INT TERM
${CC:-cc} -std=c99 -g -O1 -ffunction-sections -fdata-sections \
    -fsanitize=undefined -fno-sanitize-recover=all \
    -Itests/include -Isource/include tests/close.c \
    -Wl,--gc-sections -o "$bin"
"$bin"
