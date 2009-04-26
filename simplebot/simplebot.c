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

/* configuration stuff */
#define COMMAND_CHAR '!'
#define COMMANDS_DIR "simplebot_cmds"

/* other #defines */
#define SOCK_BUF_SZ 1024
#define PING_TIME (60*5)
#define MAX_MSG_LEN 400
#define IRC_MAX_ARGS 10
#define COMMAND_BUFFER_LENGTH 512

struct Buffer_char ircBuf, sockBuf;

void ircRead(int, short, void *);
void handlePrivmsg(int argc, char **args);
void sockRead(int, short, void *);
void ping(int, short, void *);

int main(int argc, char **argv)
{
    struct sockaddr_un sun;
    int sock, tmpi;
    struct event ircEv, sockEv, ptimer;
    struct timeval tv;

    if (argc != 3) {
        fprintf(stderr, "Use: simplebot <user> <channel>\n");
        return 1;
    }

    channel = argv[2];
    INIT_BUFFER(ircBuf);
    INIT_BUFFER(sockBuf);

    printf("USER %s localhost localhost :SimpleBot\r\n"
           "NICK :%s\r\n",
           argv[1], argv[1]);
    fflush(stdout);

    /* now open the simpleing socket */
    SF(sock, socket, -1, (AF_UNIX, SOCK_DGRAM, 0));
    sun.sun_family = AF_UNIX;
    snprintf(sun.sun_path, sizeof(sun.sun_path), "/tmp/simplebot.%s", channel);
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

                } else if (!strcmp(args[1], "PRIVMSG")) {
                    handlePrivmsg(argc, args);

                }
            }
        }

        /* now skip this line */
        skip = endl + 2 - ircBuf.buf;
        ircBuf.bufused -= skip;
        memmove(ircBuf.buf, endl + 2, ircBuf.bufused + 1);
    }
}

void handlePrivmsg(int argc, char **args)
{
    int i;
    char *msg, *msgarg, *nick, *ident, *host;
    char buf[COMMAND_BUFFER_LENGTH];

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

    /* check if this is a command */
    if (argc >= 4) {
        msg = args[3];
        if (msg[0] == COMMAND_CHAR) {
            msg++;

            /* sanitize it */
            for (i = 0; msg[i]; i++) {
                if (msg[i] == ' ') {
                    msg[i++] = '\0';
                    break;
                } else if ((msg[i] < 'A' || msg[i] > 'Z') &&
                           (msg[i] < 'a' || msg[i] > 'z')) {
                    msg[i] = '_';
                }
            }
            msgarg = msg + i;

            /* get the command */
            snprintf(buf, COMMAND_BUFFER_LENGTH, COMMANDS_DIR "/%s", msg);

            /* and run it */
            if (access(buf, X_OK) == 0) {
                if (fork() == 0) {
                    execl(buf, buf,
                          nick, ident, host, args[2] + 1, msg, msgarg,
                          args[0], args[1], args[2], args[3],
                          NULL);
                    exit(1);
                }
            }
        }
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
        printf("PRIVMSG #%s :%.*s\r\n", channel, MAX_MSG_LEN, sockBuf.buf);
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
