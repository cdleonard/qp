#! /bin/bash

set -e

: "${OUT_DIR:=./build_conan}"

mkdir -p "$OUT_DIR"
(
    cd "$OUT_DIR"
    conan install ..
)
cmake -DUSE_CONAN=1 -S . -B "$OUT_DIR"
cmake --build "$OUT_DIR"

"$OUT_DIR/bin/main_test" "$@"
