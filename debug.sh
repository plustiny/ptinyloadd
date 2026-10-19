#!/bin/bash
exec 2>&1
export DEBUG=yes
export PORT=1881
export NAME=foo
export PASSWD=bar
export PASSWD_MD5=37b51d194a7513e45b56f6524f2d51f2
export DL_DIR=/var/tmp/
export SCRIPT_DIR=./libexec/ptinyloadd
export FORCE_HTTPS=no
KEYFILE=/etc/letsencrypt/live/*/privkey.pem
CERTFILE=/etc/letsencrypt/live/*/fullchain.pem
[ -e $KEYFILE ] && [ ! -e $(basename $KEYFILE) ] && ln -s $KEYFILE ./
[ -e $CERTFILE ] && [ ! -e $(basename $CRETFILE) ] && ln -s $CERTFILE ./
export CERT=./fullchain.pem
export KEY=./privkey.pem

if command -v gdb >/dev/null; then
 gdb -q -ex run ./ptinyloadd
else
 ./ptinyloadd |& tee log
fi

