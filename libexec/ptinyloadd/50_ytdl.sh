#!/bin/bash
# @name					ytdl
# @description	download video from youtube
# @match 				"http[s]?://[^/]*(twimg|nico|youtu|x\.com|bilibili)"
require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "Error: command not found: $1" >&2
        exit 1
    }
}

require yt-dlp
url="$@"
cd "${DL_DIR}" && yt-dlp \
	--no-cache-dir -f "bestvideo[height<=1080]+bestaudio/best[height<=1080]/best" \
	--ignore-no-formats-error \
	--write-subs --write-auto-sub --sub-lang ja,en --convert-subs srt \
	--write-comments \
	--newline "${url}"

