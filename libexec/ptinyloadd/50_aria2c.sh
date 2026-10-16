#!/bin/bash
# @name					aria2c
# @description	download torrent/magnet
# @match 				"(^http[s]?://[^\s]+\.torrent|^magnet:[^\s]+)"
if ! command -v aria2c  >/dev/null; then
	echo aria2c is not installed.
	exit 1
fi
url="$@"
( cd ${DL_DIR} && exec aria2c "${url}" )
