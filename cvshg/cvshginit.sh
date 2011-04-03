#!/bin/bash
die()
{
    echo "$1"
    exit 1
}

if [ ! -e "CVS" ]
then
    echo 'This does not appear to be a CVS checkout!'
    exit 1
fi

cp `dirname "$0"`/cvshgignore .hgignore || die 'Failed to import .hgignore.'
hg init || die 'Failed to hg init.'
hg addremove || die 'Failed to hg addremove.'
hg commit -m 'Initial import.' || die 'Failed initial import.'
