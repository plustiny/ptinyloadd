#!/bin/bash
# @name					aria2c
# @description	download torrent/magnet
# @match 				"(.torrent$|^magnet:)"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}
on_error(){
    echo $1 $2 $3
    rm -f "$3"
}

require aria2c
err=""
url="$@"
aria2c -d "${DL_DIR}" \
    --on-download-complete=exit \
    --on-bt-download-complete=exit \
    --seed-time=0 \
    --on-download-error=on_error \
    --log-level=error \
    --summary-interval=10 \
    "${url}" 2>&1  | while IFS= read -r line;
do
    if [[ $line == ^Exception ]]; then
        err=$line
    fi
    line=${line//=/}
    line=${line//-/}
    [ -n "$line" ] && echo "$line"
done
status=${PIPESTATUS[0]}
[ -n "$err" ] && echo "$err"
exit $status

