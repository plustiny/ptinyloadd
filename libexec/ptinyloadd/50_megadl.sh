#!/bin/bash
# @name					megadl
# @description	download from mega
# @match 				"http[s]?://[^/]*mega.nz"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}

require megadl
megadl --path "${DL_DIR}" "$@"

