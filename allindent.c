#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "helpers.h"

#define READBUFSZ       4096
#define INDENTERS       "([{"
#define UNINDENTERS     "}])"
#define STRINGOPEN      "\""
#define STRINGCLOSE     "\""
#define STRINGESCAPE    "\\"
#define COMMENTOPEN     "/*"
#define COMMENTCLOSE    "*/"
#undef  COMMENTESCAPE
#define MAGICSTART      "#"
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
    char *readbuf, *shortbuf;

    char indentc = ' ';
    int indentper = 4;
    struct Indenter *indenter, *lindent, *nindent;
    int indentation, lnum;
    int instring, incomment, sinstring, sincomment;

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
                fprintf(stderr,
                    "Use: allindent [options] [input file] [output file]\n"
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
    instring = incomment = 0;

    /* start reading */
    for (lnum = 0; fgets(readbuf, READBUFSZ, inf); lnum++) {
        indentation = 0;
        if (indenter) indentation = indenter->indentation;
        lindent = indenter;

        /* did we start in a string/comment? */
        sinstring = 0;
        sincomment = 0;

        /* skip past the whitespace */
        for (i = 0; readbuf[i] && strchr(WHITESPACE, readbuf[i]); i++);
        shortbuf = readbuf + i;

        /* check for a magic start */
        if (!strncmp(shortbuf, MAGICSTART, sizeof(MAGICSTART)-1)) {
            WRITE_BUFFER(outb, readbuf, strlen(readbuf));
            continue;
        }

        /* look for indenters/unindenters, string/comment openers/closers */
        for (i = 0; shortbuf[i]; i++) {
            if (instring) {
                /* did we close the string? */
                if (!strncmp(shortbuf + i, STRINGCLOSE, sizeof(STRINGCLOSE)-1)) {
                    instring--;
                    if (instring < 0) instring = 0;

                } else if (!strncmp(shortbuf + i, STRINGOPEN, sizeof(STRINGOPEN)-1)) {
                    instring++;

#ifdef STRINGESCAPE
                } else if (!strncmp(shortbuf + i, STRINGESCAPE, sizeof(STRINGESCAPE)-1)) {
                    /* skip past it */
                    i += sizeof(STRINGESCAPE) - 1;
#endif

                }

            } else if (incomment) {
                /* did we close the comment? */
                if (!strncmp(shortbuf + i, COMMENTCLOSE, sizeof(COMMENTCLOSE)-1)) {
                    incomment--;
                    if (incomment < 0) incomment = 0;

                } else if (!strncmp(shortbuf + i, COMMENTOPEN, sizeof(COMMENTOPEN)-1)) {
                    incomment++;

#ifdef COMMENTESCAPE
                } else if (!strncmp(shortbuf + i, COMMENTESCAPE, sizeof(COMMENTESCAPE)-1)) {
                    i += sizeof(COMMENTESCAPE) - 1;
#endif

                }

            } else {
                if (strchr(INDENTERS, shortbuf[i])) {
                    /* indent further */
                    SF(nindent, malloc, NULL, (sizeof(struct Indenter)));
                    nindent->next = lindent;
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
                        if (indenter == lindent) {
                            indenter = indenter->next;
                        }
                        nindent = lindent->next;
                        free(lindent);
                        lindent = nindent;
                    }

                    /* move back IF this is the first thing on the line */
                    if (i == 0) {
                        if (indenter) {
                            indentation = indenter->indentation;
                        } else {
                            indentation = 0;
                        }
                    }

                } else if (!strncmp(shortbuf + i, STRINGOPEN, sizeof(STRINGOPEN)-1)) {
                    instring++;

                } else if (!strncmp(shortbuf + i, COMMENTOPEN, sizeof(COMMENTOPEN)-1)) {
                    incomment++;

                }
            }
        }
        indenter = lindent;

        /* now output */
        if (sinstring || sincomment) {
            /* we started in a string or comment, so don't change the indentation at all */
            WRITE_BUFFER(outb, readbuf, strlen(readbuf));

        } else if (i == 1) {
            /* all this contained was the newline! Don't indent an empty newline */
            WRITE_BUFFER(outb, shortbuf, i);

        } else {
            /* create the proper indentation */
            while (BUFFER_SPACE(outb) < indentation) {
                EXPAND_BUFFER(outb);
            }
            memset(BUFFER_END(outb), indentc, indentation);
            STEP_BUFFER(outb, indentation);
            WRITE_BUFFER(outb, shortbuf, i);

        }
    }
    WRITE_BUFFER(outb, "\0", 1);
    fclose(inf);

    /* now, output everything */
    if (outfname == NULL) {
        outf = stdout;
    } else {
        SF(outf, fopen, NULL, (outfname, "w"));
    }

    fputs(outb.buf, outf);
    fclose(outf);

    return 0;
}
