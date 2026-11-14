#!/bin/sh
# build.sh -- build ./fluid on macOS or Linux with any C11 compiler. -fwrapv is required: the integer laws assume
# 32-bit wraparound.
set -e
cd "$(dirname "$0")"
${CC:-cc} -std=c11 -O2 -fwrapv -w -DLOOP_LAW -DSHZX_NO_MAIN -Isrc -Isrc/codec src/fluid.c src/plat_posix.c -o fluid -lpthread
echo "built ./fluid"
