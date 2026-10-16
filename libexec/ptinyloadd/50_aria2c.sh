#!/bin/bash
# @name					aria2c
# @description	download torrent/magnet
# @match 				"(^http[s]?://[^\s]+\.torrent|^magnet:[^\s]+)"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}
require aria2c
url="$@"
cd "${DL_DIR}"
aria2c --seed-time=0 "${url}" 2>&1  | while IFS= read -r line;
do
	line=${line//=/}
	line=${line//-/}
	[ -z "$line" ] && 	printf '%s\n' "$line"
done
exit ${PIPESTATUS[0]}
