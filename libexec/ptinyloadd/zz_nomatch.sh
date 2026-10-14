#!/bin/bash
# @name					no match
# @description	no match
# @match				".*"
echo DL_DIR = $DL_DIR 
DST=${DL_DIR}/ptinyloadd_no_matched.txt
date -Is >> ${DST}
echo $@ >> ${DST}
echo no matched.

