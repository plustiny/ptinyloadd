#!/bin/bash
# @name					no_match
# @description	no match
# @match				".*"
DST="${DL_DIR}/ptinyloadd_no_matched.txt"
echo -n "# "
date -Is >> ${DST}
echo $@ >> ${DST}
echo no matched. save url to ${DST} .
