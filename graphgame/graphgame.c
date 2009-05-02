#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <gc/gc.h>

#define ISMAGIC(_nodec, _node) ((_node) > 1 && (_node) <= (_nodec)/3+1)

char *nodeName(int num)
{
    char *name;
    int numhi, numlo;
    
    numhi = num / 26;
    numlo = num % 26;
    if (numhi >= 26) numhi = 25;

    name = GC_MALLOC(3);
    name[0] = 'A' + numhi;
    name[1] = 'A' + numlo;
    name[2] = '\0';
    return name;
}

static inline int randomNonMagic(int nodec)
{
    int v;
    do {
        v = random() % (nodec - (nodec/3));
    } while (ISMAGIC(nodec, v));
    return v;
}

int main(int argc, char **argv)
{
    int nodec, edgec, i, f;
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
               ISMAGIC(nodec, i) ? "" : ", shape=\"box\"");
    }
    for (i = 0; i < nodec && i < edgec; i++) {
        printf("%s -> %s [color=\"#%02X%02X%02X\"];\n",
               nodeName(i),
               nodeName(ISMAGIC(nodec, i) ?
                   randomNonMagic(nodec) :
                   random() % nodec),
               random() % 0x80, random() % 0x80, random() % 0x80);
    }
    if (edgec >= nodec * 2) {
        /* guarantee that every node has an input */
        for (i = 0; i < nodec; i++) {
            printf("%s -> %s [color=\"#%02X%02X%02X\"];\n",
                   nodeName(ISMAGIC(nodec, i) ?
                       randomNonMagic(nodec) :
                       random() % nodec),
                   nodeName(i),
                   random() % 0x80, random() % 0x80, random() % 0x80);
        }
        i += nodec;
    }
    for (; i < edgec; i++) {
        f = random() % nodec;
        printf("%s -> %s [color=\"#%02X%02X%02X\"];\n",
               nodeName(f),
               nodeName(ISMAGIC(nodec, f) ?
                   randomNonMagic(nodec) :
                   random() % nodec),
               random() % 0x80, random() % 0x80, random() % 0x80);
    }
    printf("}\n");

    return 0;
}
