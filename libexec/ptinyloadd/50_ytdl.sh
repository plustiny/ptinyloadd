#!/bin/bash
# @name					ytdl
# @description	download video from youtube
# @match 				"http[s]?://[^/]*(twimg|nico|youtu|x\.com|bilibili)[^\s]+"
if ! command -v yt-dlp >/dev/null; then
	echo yt-dlp is not installed.
	exit 1
fi
url="$@"
( cd ${DL_DIR} && yt-dlp --no-cache-dir -f "bestvideo[height<=1080]+bestaudio/best[height<=1080]/best" --ignore-no-formats-error --write-subs --write-auto-sub --sub-lang ja,en --convert-subs srt --write-comments --newline "${url}" )

