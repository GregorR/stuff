#!/bin/bash
die()
{
    echo "$1"
    exit 1
}

COMMIT=`dirname "$0"`/cvshgcommit.sh

while $COMMIT; do true; done
