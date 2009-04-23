#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <event.h>

#include "buffer.h"

char *channel;

#define SOCK_BUF_SZ 1024
#define PING_TIME (60*5)

struct Buffer_char ircBuf, sockBuf;

void ircRead(int, short, void *);
void sockRead(int, short, void *);
void ping(int, short, void *);

int main(int argc, char **argv)
{
    struct sockaddr_un sun;
    int sock, tmpi;
    struct event ircEv, sockEv, ptimer;
    struct timeval tv;

    if (argc != 3) {
        fprintf(stderr, "Use: forwardbot <user> <channel>\n");
        return 1;
    }

    channel = argv[2];
    INIT_BUFFER(ircBuf);
    INIT_BUFFER(sockBuf);

    printf("USER %s localhost localhost :ForwardBot\r\n"
           "NICK :%s\r\n",
           argv[1], argv[1]);
    fflush(stdout);

    /* now open the forwarding socket */
    SF(sock, socket, -1, (AF_UNIX, SOCK_DGRAM, 0));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "/tmp/forwardbot.%s", channel);
    unlink(sun.sun_path);
    SF(tmpi, bind, -1, (sock, (struct sockaddr *) &sun, sizeof(sun)));

    /* make everything nonblocking */
    fcntl(0, F_SETFL, O_NONBLOCK);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    /* set up libevent */
    event_init();

    event_set(&ircEv, 0, EV_READ|EV_PERSIST, ircRead, NULL);
    event_add(&ircEv, NULL);

    event_set(&sockEv, sock, EV_READ|EV_PERSIST, sockRead, NULL);
    event_add(&sockEv, NULL);

    tv.tv_sec = PING_TIME;
    tv.tv_usec = 0;
    evtimer_set(&ptimer, ping, NULL);
    evtimer_add(&ptimer, &tv);

    return event_loop(0);
}

void ircRead(int fd, short i0, void *i1)
{
    int rd, skip;
    char *endl, *spc;
    static int joined = 0;

    /* get this input */
    while ((rd = recv(fd, ircBuf.buf + ircBuf.bufused, ircBuf.bufsz - ircBuf.bufused, 0)) > 0) {
        ircBuf.bufused += rd;
        if (ircBuf.bufused >= ircBuf.bufsz)
            EXPAND_BUFFER(ircBuf);
    }
    ircBuf.buf[ircBuf.bufused] = '\0';

    /* look for \r\n */
    while ((endl = strstr(ircBuf.buf, "\r\n"))) {
        *endl = '\0';

        /* perhaps this is time to join the channel */
        if (!joined && ircBuf.buf[0] == ':') {
            joined = 1;
            printf("JOIN #%s\r\n", channel);
            fflush(stdout);
        }

        /* look for the command */
        if ((spc = strchr(ircBuf.buf, ' '))) {
            spc++;

            /* check if it's PING */
            if (!strncmp(spc, "PING", 4)) {
                /* pong! */
                printf("PONG :localhost\r\n");
                fflush(stdout);
            }
        }

        /* now skip this line */
        skip = endl + 2 - ircBuf.buf;
        ircBuf.bufused -= skip;
        memmove(ircBuf.buf, endl + 2, ircBuf.bufused + 1);
    }
}

void sockRead(int fd, short i0, void *i1)
{
    int rd, skip;
    char *endl;

    /* get this input */
    while ((rd = read(fd, sockBuf.buf + sockBuf.bufused, sockBuf.bufsz - sockBuf.bufused)) > 0) {
        sockBuf.bufused += rd;
        if (sockBuf.bufused >= sockBuf.bufsz)
            EXPAND_BUFFER(sockBuf);
    }
    sockBuf.buf[sockBuf.bufused] = '\0';

    /* look for \n */
    while ((endl = strchr(sockBuf.buf, '\n'))) {
        *endl = '\0';

        /* print it out */
        printf("PRIVMSG #%s :%s\r\n", channel, sockBuf.buf);
        fflush(stdout);

        /* now skip this line */
        skip = endl + 1 - sockBuf.buf;
        sockBuf.bufused -= skip;
        memmove(sockBuf.buf, endl + 1, sockBuf.bufused + 1);
    }
}

void ping(int i0, short i1, void *i2)
{
    printf("PING :localhost\r\n");
    fflush(stdout);
}
