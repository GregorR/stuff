#!/bin/bash
die()
{
    echo "$1"
    exit 1
}

if [ ! -e "CVS" -o ! -e ".hg" ]
then
    echo 'This does not appear to be a CVS-hg directory!'
    exit 1
fi

cvs up -d || die 'Failed to cvs up.'
hg addr || die 'Failed to hg addr.'
hg commit -m 'cvs up' || die 'Failed to hg commit.'
