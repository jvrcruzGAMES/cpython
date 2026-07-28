#!/bin/sh
set -eu

runtime_dir=$1

native_files=$(find "$runtime_dir" -type f \( \
    -name '*.so' -o \
    -name '*.pyd' -o \
    -name '*.nro' -o \
    -name '*.nso' -o \
    -name '*.elf' \
    \) -print)

if [ -n "$native_files" ]; then
    printf '%s\n' "$native_files" >&2
    echo "Native Python payloads cannot be packed into romfs; link them or package them in an executable title layout." >&2
    exit 1
fi
