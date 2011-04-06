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

HGREV=0
if [ "$1" ]
then
    OHGREV="tip"
    HGREV="$1"
else
    OHGREV=`hg id -n || die "Failed to get hg revision."`
    HGREV=$((OHGREV + 1))
    echo "Detected revision $HGREV"
fi

hg up -r"$HGREV" || die "Failed to hg up"

if [ `hg id -n` != "$HGREV" ]
then
    hg up -r"$OHGREV"
    die "I'm not on revision $HGREV at all"'!'
fi

# Add/remove files
ADDR=no
for f in `hg log -p -r"$HGREV" | grep -E '^(---|\+\+\+)' | sed 's/^[^ ]* // ; s/\t.*//'`
do
    if [ "$f" = "/dev/null" ]
    then
        ADDR=yes
        break
    fi
done
if [ "$ADDR" = "yes" ]
then
    echo 'Files have been added or removed. Please add or remove them to/from the CVS repository manually.'
    bash || exit 1
fi

# Then commit
hg log -r"$HGREV" --style compact | perl -pe 's/\n// ; s/  */ /g' > /tmp/log.$$ || die 'Failed to get log message.'
cvs commit -F/tmp/log.$$ || die 'Failed to commit with /tmp/log.'$$
rm -f /tmp/log.$$
