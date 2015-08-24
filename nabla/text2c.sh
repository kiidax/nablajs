#!/bin/sh

if true ; then
    echo "namespace nabla {"
    echo "namespace internal {"
    echo "const char* startup_source = "
    /usr/bin/sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/.*/"&\\n"/' $1
    echo "; }} "
fi > $2
