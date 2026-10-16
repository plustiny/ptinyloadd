#!/bin/bash
# @name					aria2c
# @description	download torrent/magnet
# @match 				"(.torrent$|.meta4$|.metalink$|^magnet:)"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}
require aria2c
url="$@"
aria2c -d "${DL_DIR}" --on-download-complete --log-level=info "${url}" 2>&1  | while IFS= read -r line;
do
	line=${line//=/}
	line=${line//-/}
	[ -n "$line" ] && 	printf '%s\n' "$line"
done
exit ${PIPESTATUS[0]}
