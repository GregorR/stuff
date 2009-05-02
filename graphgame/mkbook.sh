#!/bin/bash
if [ ! "$3" ]
then
    echo 'Use: mkbook.sh <number of nodes> <number of edges> <pages>' >&2
    exit 1
fi

NODES="$1"
EDGES="$2"
PREFIX="${NODES}_${EDGES}"
PAGES="$3"
TITLE="GraphGame $NODES/$EDGES"
GGMD5="// `md5sum graphgame.c`"

# First generate them
for (( i = 1; $i <= $PAGES; i++ ))
do
    echo $i >&2
    if [ ! -e ${PREFIX}_${i}.dot ]
    then
        (
            ./graphgame $NODES $EDGES $i
            echo "$GGMD5"
        ) > ${PREFIX}_${i}.dot
    fi
done

echo '\documentclass{article} \usepackage{graphicx} \usepackage[landscape,margin=0in]{geometry} \begin{document}
\title{\Huge{'${TITLE}'}} \date{} \maketitle \newpage' > ${PREFIX}_book.tex

for (( i = 1; $i <= $PAGES; i++ ))
do
    make ${PREFIX}_${i}.dpdf >&2
    mv -f ${PREFIX}_${i}.dpdf ${PREFIX}_${i}_dot.pdf

    echo '\includegraphics[width=10.5in,height=8in]{'${PREFIX}'_'$i'_dot.pdf}\newpage' >> ${PREFIX}_book.tex
done

echo '\end{document}' >> ${PREFIX}_book.tex

pdflatex ${PREFIX}_book.tex
rm -f ${PREFIX}_book.{aux,log} ${PREFIX}_*_dot.pdf &&
tar jcf ${PREFIX}_book.tar.bz2 ${PREFIX}_*.dot ${PREFIX}_*.pdf ${PREFIX}_book.tex &&
rm -f ${PREFIX}_*.dot ${PREFIX}_*_dot.pdf ${PREFIX}_book.tex
