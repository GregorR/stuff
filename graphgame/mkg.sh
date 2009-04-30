#!/bin/bash
if [ ! "$2" ]
then
    echo 'Use: mkg.sh <number of nodes> <number of edges>' >&2
    exit 1
fi

./graphgame $1 $2 > ${1}_${2}.dot
make ${1}_${2}.pdf clean
