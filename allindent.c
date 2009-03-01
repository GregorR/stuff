#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "helpers.h"

#define READBUFSZ       4096
#define INDENTERS       "([{"
#define UNINDENTERS     "}])"
#define WHITESPACE      " \t"

struct Indenter {
    struct Indenter *next;
    int linenum, indentation;
};

int main(int argc, char **argv)
{
    struct Buffer_char outb;
    FILE *inf, *outf;
    char *infname = NULL, *outfname = NULL;
    int i;
    char *readbuf, *shortbuf, *outindent;

    char indentc = ' ';
    int indentper = 4;
    struct Indenter *indenter, *lindent, *nindent;
    int indentation, lnum;

    /* read the arguments */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (i < argc - 1) {
                if (!strcmp(argv[i], "--indent-char") || !strcmp(argv[i], "-c")) {
                    indentc = argv[++i][0];
                    continue;

                } else if (!strcmp(argv[i], "--indent-width") || !strcmp(argv[i], "-w")) {
                    indentper = atoi(argv[++i]);
                    continue;

                }
            }

            if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
                fprintf(stderr, "Use: allindent [options] [input file] [output file]\n"
                                "Options:\n"
                                "  --indent-char|-c <character>: Character to use for indentation\n"
                                "  --use-spaces|-s: Use ' ' for indentation (the default)\n"
                                "  --use-tabs|-t: Use tabs for indentation\n"
                                "  --indent-width|-w <number>: Width to indent (must come after -s or -t)\n");
                return 0;

            } else if (!strcmp(argv[i], "--use-spaces") || !strcmp(argv[i], "-s")) {
                indentc = ' ';
                indentper = 4;

            } else if (!strcmp(argv[i], "--use-tabs") || !strcmp(argv[i], "-t")) {
                indentc = '\t';
                indentper = 1;

            } else {
                fprintf(stderr, "Unrecognized argument %s\n", argv[i]);
                return 1;

            }

        } else {
            if (infname == NULL) {
                infname = argv[i];

            } else if (outfname == NULL) {
                outfname = argv[i];

            } else {
                fprintf(stderr, "Unexpected argument %s\n", argv[i]);
                return 1;

            }

        }
    }

    /* sanity */
    if (indentper < 0) indentper = 0;

    /* open the input file ... */
    if (infname == NULL) {
        inf = stdin;
    } else {
        SF(inf, fopen, NULL, (infname, "r"));
    }

    SF(readbuf, malloc, NULL, (READBUFSZ));
    INIT_BUFFER(outb);
    indenter = NULL;

    /* start reading */
    for (lnum = 0; fgets(readbuf, READBUFSZ, inf); lnum++) {
        indentation = 0;
        if (indenter) indentation = indenter->indentation;
        lindent = indenter;

        /* skip past the whitespace */
        for (i = 0; readbuf[i] && strchr(WHITESPACE, readbuf[i]); i++);
        shortbuf = readbuf + i;

        /* look for indenters/unindenters */
        for (i = 0; shortbuf[i]; i++) {
            if (strchr(INDENTERS, shortbuf[i])) {
                /* indent further */
                SF(nindent, malloc, NULL, (sizeof(struct Indenter)));
                nindent->next = indenter;
                nindent->linenum = lnum;

                /* check the current indentation */
                if (lindent) {
                    if (lindent->linenum == lnum) {
                        nindent->indentation = lindent->indentation;
                    } else {
                        nindent->indentation = lindent->indentation + indentper;
                    }
                } else {
                    nindent->indentation = indentper;
                }
                lindent = nindent;

            } else if (strchr(UNINDENTERS, shortbuf[i])) {
                /* unindent */
                if (lindent) {
                    if (indenter == lindent)
                        indenter = indenter->next;
                    nindent = lindent->next;
                    free(lindent);
                    lindent = nindent;
                }

                if (indenter) {
                    indentation = indenter->indentation;
                } else {
                    indentation = 0;
                }
            }
        }
        indenter = lindent;

        /* now output */
        SF(outindent, malloc, NULL, (indentation));
        memset(outindent, indentc, indentation);
        WRITE_BUFFER(outb, outindent, indentation);
        free(outindent);
        WRITE_BUFFER(outb, shortbuf, i);
    }
    WRITE_BUFFER(outb, "\0", 1);
    fclose(inf);

    /* now, output everything */
    if (outfname == NULL) {
        outf = stdout;
    } else {
        SF(outf, fopen, NULL, (outfname, "r"));
    }

    fputs(outb.buf, outf);
    fclose(outf);

    return 0;
}
