CC=gcc
CFLAGS=-O2 -g
LDFLAGS=
LIBS=-lgc

all: graphgame

graphgame: graphgame.c
	$(CC) $(CFLAGS) $(LDFLAGS) graphgame.c $(LIBS) -o graphgame

.SUFFIXES: .dot .dpdf .pdf

.dot.dpdf:
	fdp $< -Gsplines=true -Tpdf -o $@

.dpdf.pdf:
	ln -f $< $*_dot.pdf
	echo '\documentclass{article} \usepackage{graphicx} \usepackage[landscape,margin=0in]{geometry} \begin{document} \includegraphics[width=10.5in,height=8in]{$*_dot.pdf} \end{document}' > $*.tex
	pdflatex $*.tex

clean:
	rm -f *.dpdf *_dot.pdf *.tex *.aux *.log
