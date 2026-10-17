#!/bin/bash
# @name					no match
# @description	no match
# @match				".*"
DST="${DL_DIR}/ptinyloadd_no_matched.txt"
date -Is >> ${DST}
echo $@ >> ${DST}
echo no matched. save url to ${DST} .
