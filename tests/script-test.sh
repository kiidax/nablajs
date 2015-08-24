#!/bin/sh

for jsfile in *.js ; do
    echo "Testing $jsfile..."
    tmpfile=`mktemp`
    rspfile=${jsfile%.js}.rsp
    ../nabla/nabla $jsfile | sed -e 's/\r$//' > $tmpfile
    cmp $rspfile $tmpfile
    res=$?
    rm $tmpfile
    test $res -eq 0 || exit $res
done

exit 0
