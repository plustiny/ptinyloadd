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
if [[ "$@" == *"/folder/"* ]];then
    FOLDER=$(yes ""|megadl --choose-files "$@" 2>/dev/null |head -n1 |sed 's/^[0-9]*\.[ ]*//')
    DL_DIR="${DL_DIR}/${FOLDER}"
    mkdir -p "${DL_DIR}"
fi
megadl --path "${DL_DIR}" "$@"

