#!/bin/bash
# @name         test re-post
# @description	test re-post
# @match		^test2$
echo "pid= $$, token= $TOKEN"
sleep 2
echo curl -X POST -d test_re -u "$$:${TOKEN}" http://localhost:${LOCAL_PORT}
sleep 2
curl -X POST -d test_re -u "$$:${TOKEN}" http://localhost:${LOCAL_PORT} 2>/dev/null
echo "exit stat $?"
