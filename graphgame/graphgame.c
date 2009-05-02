#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <gc/gc.h>

#define ISMAGIC(_node) ((_node) > 1 && (_node) <= nodec/3+1)

int nodec;

char *nodeName(int num)
{
    char *name;
    int numhi, numlo;
    
    numhi = num / 26;
    numlo = num % 26;
    if (numhi >= 26) numhi = 25;

    name = GC_MALLOC_ATOMIC(3);
    name[0] = 'A' + numhi;
    name[1] = 'A' + numlo;
    name[2] = '\0';
    return name;
}

static inline int randomNonMagic()
{
    int v;
    do {
        v = random() % (nodec - (nodec/3));
    } while (ISMAGIC(v));
    return v;
}

void drawEdge(int fromo, int too)
{
    int fromn, ton;
    char *from, *to;
    static char *haveLink = NULL;

    /* keep track of the links we already have to avoid repetition */
    if (haveLink == NULL) {
        haveLink = GC_MALLOC_ATOMIC(nodec * nodec);
        memset(haveLink, 0, nodec * nodec);
    }

retry:
    fromn = fromo;
    ton = too;

    if (fromn < 0) {
        if (ISMAGIC(ton))
            fromn = randomNonMagic();
        else
            fromn = random() % nodec;
    }

    if (ton < 0) {
        if (ISMAGIC(fromn))
            ton = randomNonMagic();
        else
            ton = random() % nodec;
    }

    if (fromn == ton ||
        (fromn <= 1 && ton <= 1) ||
        haveLink[fromn*nodec+ton]) goto retry;
    haveLink[fromn*nodec+ton] = 1;
    from = nodeName(fromn);
    to = nodeName(ton);

    printf("%s -> %s [color=\"#%02X%02X%02X\"];\n",
           from, to,
           random() % 0x80, random() % 0x80, random() % 0x80);
}

int main(int argc, char **argv)
{
    int edgec, i, f;
    unsigned int seed;

    if (argc < 3) {
        fprintf(stderr, "Use: graphgame <number of nodes> <number of edges> [random seed]\n");
        return 1;
    }

    nodec = atoi(argv[1]);
    edgec = atoi(argv[2]);

    if (argc < 4) {
        seed = time(NULL) ^ getpid();
    } else {
        seed = (unsigned) atoi(argv[3]);
    }
    srandom(seed);

    printf("digraph {\n"
           "// nodes: %d, edges: %d, seed: %u\n"
           "AA [label=\"\", shape=\"parallelogram\", style=\"filled\", bgcolor=\"#AAAAAA\"];\n"
           "AB [label=\"\", shape=\"trapezium\", style=\"filled\", bgcolor=\"#AAAAAA\"];\n",
           nodec, edgec, seed);
    for (i = 2; i < nodec; i++) {
        printf("%s [label=\"\"%s];\n", nodeName(i),
               ISMAGIC(i) ? "" : ", shape=\"box\"");
    }
    for (i = 0; i < nodec && i < edgec; i++) {
        drawEdge(i, -1);
    }
    if (edgec >= nodec * 2) {
        /* guarantee that every node has an input */
        for (i = 0; i < nodec; i++) {
            drawEdge(-1, i);
        }
        i += nodec;
    }
    for (; i < edgec; i++) {
        drawEdge(-1, -1);
    }
    printf("}\n");

    return 0;
}
