#!/bin/bash
export DEBUG=yes
export PORT=1881
export LPORT=1882
export UNAME=foo
export PASSWD_MD5=37b51d194a7513e45b56f6524f2d51f2
export DL_DIR=/var/tmp/
export SCRIPT_DIR=./libexec/ptinyloadd
export FORCE_HTTPS=no
export NO_LOCAL_AUTH=no
KEYFILE=/etc/letsencrypt/live/*/privkey.pem
CERTFILE=/etc/letsencrypt/live/*/fullchain.pem
[ -e $KEYFILE ] && [ ! -e $(basename $KEYFILE) ] && ln -s $KEYFILE ./
[ -e $CERTFILE ] && [ ! -e $(basename $CRETFILE) ] && ln -s $CERTFILE ./
export CERT=./fullchain.pem
export KEY=./privkey.pem

if command -v gdb >/dev/null; then
 gdb -ex 'set confirm off' -ex run --args ./ptinyloadd
else
 ./ptinyloadd 2>&1| tee /tmp/log
fi

