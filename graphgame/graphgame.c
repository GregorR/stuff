#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gc/gc.h>

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

int main(int argc, char **argv)
{
    int nodec, edgec, i;

    if (argc != 3) {
        fprintf(stderr, "Use: graphgame <number of nodes> <number of edges>\n");
        return 1;
    }

    nodec = atoi(argv[1]);
    edgec = atoi(argv[2]);

    srandom(time(NULL));

    printf("digraph {\n"
           "AA [style=\"filled\", bgcolor=\"#DDDDDD\"];\n");
    for (i = 1; i < nodec; i++) {
        printf("%s;\n", nodeName(i));
    }
    for (i = 0; i < edgec; i++) {
        printf("%s -> %s [color=\"#%02X%02X%02X\"];\n",
               nodeName(random() % nodec), nodeName(random() % nodec),
               random() % 0x80, random() % 0x80, random() % 0x80);
    }
    printf("}\n");

    return 0;
}
