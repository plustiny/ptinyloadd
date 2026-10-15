#!/bin/bash
export DEBUG=true
export PORT=1881
export NAME=foo
export PASSWD=bar
export PASSWD_MD5=37b51d194a7513e45b56f6524f2d51f2
export DL_DIR=/var/tmp/
export SCRIPT_DIR=./libexec/ptinyloadd
ln -s /etc/letsencrypt/live/*/{privkey.pem,fullchain.pem} ./
export CERT=./fullchain.pem
export KEY=./privkey.pem

if command -v gdb >/dev/null; then
 gdb -q -ex run ./ptinyloadd
else
 ./ptinyloadd 
fi

