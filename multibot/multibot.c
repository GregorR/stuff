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

char *nick, *channel, *sockName;

/* configuration stuff */
#define COMMANDS_DIR    "multibot_cmds"
#define SOCK_NAME       "/tmp/multibot.%s"

/* other #defines */
#define SOCK_BUF_SZ 1024
#define PING_TIME (60*5)
#define MAX_MSG_LEN 512
#define IRC_MAX_ARGS 10

struct Buffer_char ircBuf, sockBuf;

void ircRead(int, short, void *);
void handleMessage(int argc, char **args);
void sockRead(int, short, void *);
void ping(int, short, void *);

int main(int argc, char **argv)
{
    struct sockaddr_un sun;
    int sock, tmpi;
    struct event ircEv, sockEv, ptimer;
    struct timeval tv;

    if (argc != 3) {
        fprintf(stderr, "Use: multibot <user> <channel>\n");
        return 1;
    }

    nick = argv[1];
    channel = argv[2];
    INIT_BUFFER(ircBuf);
    INIT_BUFFER(sockBuf);

    printf("USER %s localhost localhost :MultiBot\r\n"
           "NICK :%s\r\n",
           nick, nick);
    fflush(stdout);

    /* name the sock */
    sockName = malloc(sizeof(SOCK_NAME) + strlen(nick));
    sprintf(sockName, SOCK_NAME, nick);

    /* now open the socket */
    SF(sock, socket, -1, (AF_UNIX, SOCK_DGRAM, 0));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", sockName);
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
    int rd, skip, argc;
    char *endl, *args[IRC_MAX_ARGS];
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

        if (ircBuf.buf[0] == ':') {
            /* perhaps this is time to join the channel */
            if (!joined) {
                joined = 1;
                printf("JOIN #%s\r\n", channel);
                fflush(stdout);
            }

            /* separate out the pieces */
            args[0] = ircBuf.buf;
            for (argc = 1; argc < IRC_MAX_ARGS; argc++) {
                if ((args[argc] = strchr(args[argc-1], ' '))) {
                    *(args[argc]++) = '\0';
                } else {
                    break;
                }

                /* if this is the final arg, don't keep reading */
                if (args[argc][0] == ':') {
                    args[argc++]++;
                    break;
                }
            }

            /* check for commands */
            if (argc >= 2) {
                if (!strcmp(args[1], "PING")) {
                    /* pong! */
                    printf("PONG :localhost\r\n");
                    fflush(stdout);

                } else {
                    handleMessage(argc, args);

                }
            }
        }

        /* now skip this line */
        skip = endl + 2 - ircBuf.buf;
        ircBuf.bufused -= skip;
        memmove(ircBuf.buf, endl + 2, ircBuf.bufused + 1);
    }
}

void handleMessage(int argc, char **args)
{
    int i, j, found;
    char *nick, *ident, *host;
    char *newargs[IRC_MAX_ARGS + 2];
    struct Buffer_char cmdname;

    /* separate out the nick/ident/host */
    nick = args[0] + 1;
    if ((ident = strchr(nick, '!'))) {
        *(ident++) = '\0';
    } else {
        ident = nick;
    }
    if ((host = strchr(ident, '@'))) {
        *(host++) = '\0';
    } else {
        host = ident;
    }

    /* now copy over the args */
    memcpy(newargs + 1, args + 1, (argc - 1) * sizeof(char*));
    newargs[argc] = NULL;

    /* prepare to look for a program */
    INIT_BUFFER(cmdname);
    WRITE_BUFFER(cmdname, COMMANDS_DIR, sizeof(COMMANDS_DIR)-1);
    WRITE_BUFFER(cmdname, "/", 1);

    /* look from most to least specific */
    found = 0;
    for (i = 2; i >= 1; i--) {
        cmdname.bufused = sizeof(COMMANDS_DIR);

        /* copy in the command name */
        for (j = 1; argc > j && j <= i; j++) {
            WRITE_BUFFER(cmdname, args[j], (j == 3) ? 1 : strlen(args[j]));
            if (j != i) WRITE_BUFFER(cmdname, "/", 1);
        }

        /* scrub it */
        for (j = sizeof(COMMANDS_DIR); j < cmdname.bufused; j++) {
            char c = cmdname.buf[j];
            if ((c < 'A' || c > 'Z') &&
                (c < 'a' || c > 'z') &&
                (c < '0' || c > '9') &&
                c != '_' && c != '/' && c != '.')
                cmdname.buf[j] = '_';
        }

        /* first check with the start of args[3] (the message) */
        if (argc > 3) {
            char hex[3];
            snprintf(hex, 3, "%02X", args[3][0]);
            WRITE_BUFFER(cmdname, "/tr_", 4);
            WRITE_BUFFER(cmdname, hex, 2);
            WRITE_BUFFER(cmdname, ".cmd", 5);

            if (access(cmdname.buf, X_OK) == 0) {
                found = 1;
                break;
            }

            cmdname.bufused -= 11; /* /tr_hx.cmd\0 */
        }

        /* then check with just .cmd */
        WRITE_BUFFER(cmdname, ".cmd", 5);
        if (access(cmdname.buf, X_OK) == 0) {
            found = 1;
            break;
        }
    }

    /* and run it */
    if (found) {
        newargs[0] = cmdname.buf;
        if (fork() == 0) {
            /* set helpful environment vars */
            setenv("IRC_SOCK", sockName, 1);
            setenv("IRC_NICK", nick, 1);
            setenv("IRC_IDENT", ident, 1);
            setenv("IRC_HOST", host, 1);

            chdir(COMMANDS_DIR);
            execv(newargs[0] + sizeof(COMMANDS_DIR), newargs);
            exit(1);
        }
    }

    FREE_BUFFER(cmdname);
}

void sockRead(int fd, short i0, void *i1)
{
    int rd, skip;
    char *endl, *cr;

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

        /* clear any tricky \rs */
        while ((cr = strchr(sockBuf.buf, '\r'))) *cr = '\0';

        /* print it out */
        printf("%.*s\r\n", MAX_MSG_LEN, sockBuf.buf);
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
