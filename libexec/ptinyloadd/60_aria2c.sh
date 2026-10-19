#!/bin/bash
# @name					aria2c
# @description	download http/torrent/magnet
# @match 				"(.torrent$|^magnet:|^https?://)"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}

require aria2c
aria2c -d "${DL_DIR}" \
    -c \
    --allow-overwrite=true \
    --seed-time=0 \
    --log-level=error \
    --summary-interval=10 \
    "$@"

