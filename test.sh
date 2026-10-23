TMP=/tmp/test.ptinyloadd
rm -f $TMP
CURL="curl -s -o $TMP -w %{http_code}"  

echo test meta
status=$($CURL -X GET http://foo:bar@localhost:1881/?query=meta )
if [ "$status" = "200" ];then
	echo [OK] 401 OK
else
	echo [ERR] $status
fi
cat $TMP
echo

echo test
status=$($CURL -X POST http://foo:bar@localhost:1881 -d bas)
if [ "$status" = "202" ];then
	echo [OK] 202 Accepted
else
	echo [ERR] $status
fi

status=$($CURL -X GET http://foo:bar@localhost:1881)
if [ "$status" = "400" ];then
	echo [OK] 400 Bad Request
else
	echo [ERR] $status
fi

echo
echo "test auth"
status=$($CURL -X POST http://FOO:bar@localhost:1881 -d bas)
if [ "$status" = "401" ];then
	echo [OK] 401 Unauthorized
else
	echo [ERR] $status
fi

status=$($CURL -X POST http://foo:BAR@localhost:1881 -d bas)
if [ "$status" = "401" ];then
	echo [OK] 401 Unauthorized
else
	echo [ERR] $status
fi

echo
echo "test regist"
status=$($CURL -X POST http://foo:bar@localhost:1881 -d test_data)
if [ "$status" = "202" ];then
	echo [OK] 202 Accepted
else
	echo [ERR] $status
fi

echo
echo "test select"
status=$($CURL -X GET http://foo:bar@localhost:1881/?query=select)
if [ "$status" = "200" ];then
	echo [OK] 401 OK
else
	echo [ERR] $status
fi
cat $TMP
echo
sleep 5
echo 
status=$($CURL --compressed -X GET http://foo:bar@localhost:1881/?query=select)
cat $TMP
echo

echo
echo "test delete"
status=$($CURL -X POST http://foo:bar@localhost:1881 -d test_a)
status=$($CURL -X POST http://foo:bar@localhost:1881 -d test_b)
status=$($CURL -X GET http://foo:bar@localhost:1881/?query=select)
id=$(cat $TMP|jq -r '.[0].id')
cat $TMP
echo 
echo $id
sleep 2
status=$($CURL -X GET "http://foo:bar@localhost:1881/?query=delete&id=$id")
if [ "$status" = "200" ];then
	echo [OK] 401 OK
else
	echo [ERR] $status
fi
status=$($CURL -X GET http://foo:bar@localhost:1881/?query=select)
cat $TMP
echo

echo
echo test compressed post
status=$(echo Hello|gzip|$CURL -H 'Content-Encoding: gzip'  -X POST http://foo:bar@localhost:1881 --data-binary @-)
if [ "$status" = "202" ];then
	echo [OK] 202 Accepted
else
	echo [ERR] $status
fi

echo
echo test re-post
status=$($CURL -X POST http://foo:bar@localhost:1881 -d test2)
if [ "$status" = "202" ];then
	echo [OK] 202 Accepted
else
	echo [ERR] $status
fi
status=$($CURL --compressed -X GET http://foo:bar@localhost:1881/?query=select)
cat $TMP
echo


