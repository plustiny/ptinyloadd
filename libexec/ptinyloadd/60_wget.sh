#!/bin/bash
# @name					wget
# @description	download
# @match 				"http[s]?://"
if command -v wget >/dev/null; then
	echo wget is not installed.
	exit 1
fi
url="$@"
( cd ${DL_DIR} && wget "${url}" )

