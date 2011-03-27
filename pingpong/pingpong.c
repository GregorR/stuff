/*
 * Copyright (C) 2011 Gregor Richards
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define _BSD_SOURCE
#define _POSIX_SOURCE

#include <netdb.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "helpers.h"

void pingpong(int sock, char **cmd);

int pingTime = 300;
int pongTime = 15;
int sendTime = 15;

#define MAXCONN 1024

int main(int argc, char **argv)
{
    int server = 0;
    char *arg, *remoteHost = NULL, *port = NULL;
    char **cmd = NULL;
    int i, tmpi;
    pid_t children[MAXCONN];
    int curChild = 0;

#define NEXTARG do { \
    if (i >= argc - 1) { \
        fprintf(stderr, "Invalid invocation.\n"); \
        return 1; \
    } \
    arg = argv[++i]; \
} while (0)

    for (i = 1; i < argc; i++) {
        arg = argv[i];
        if (arg[0] == '-') {
            if (!strcmp(arg, "-d")) {
                server = 1;
            } else if (!strcmp(arg, "-i")) {
                NEXTARG;
                pingTime = atoi(arg);
            } else if (!strcmp(arg, "-o")) {
                NEXTARG;
                pongTime = atoi(arg);
            } else if (!strcmp(arg, "-s")) {
                NEXTARG;
                sendTime = atoi(arg);
            } else {
                fprintf(stderr, "Use: pingpong [-i timeout=300] [-o timeout=15] [-s timeout=15] {-d port|host port} [cmd]\n");
            }
        } else {
            if (server) {
                if (!port) {
                    port = arg;
                } else {
                    cmd = argv + i;
                    break;
                }
            } else {
                if (!remoteHost) {
                    remoteHost = arg;
                } else if (!port) {
                    port = arg;
                } else {
                    cmd = argv + i;
                    break;
                }
            }
        }
    }

    if ((!server && !remoteHost) || !port) {
        fprintf(stderr, "Invalid invocation.\n");
        return 1;
    }

    if (server) {
        int sock, conn;
        struct sockaddr_in sin;

        /* listen for connections */
        SF(sock, socket, -1, (AF_INET, SOCK_STREAM, 0));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(atoi(port));
        sin.sin_addr.s_addr = 0;
        SF(tmpi, bind, -1, (sock, (struct sockaddr *) &sin, sizeof(sin)));

        if (fork() == 0) {
            daemon(0, 0);
        } else {
            return 0;
        }

        while (!listen(sock, 32)) {
            conn = accept(sock, NULL, NULL);
            if (conn >= 0) {
                /* handle this in another fork */
                children[curChild] = fork();
                if (children[curChild] == 0) {
                    pingpong(conn, cmd);
                    return 0;
                }
                curChild++;
            }

            /* wait for children */
            for (i = 0; i < curChild; i++) {
                if (waitpid(children[i], NULL, WNOHANG) != -1) {
                    /* that child is done */
                    memmove(children + i, children + i + 1, (curChild - i - 1) * sizeof(pid_t));
                    i--;
                }
            }
        }

    } else {
        struct addrinfo *ai;
        int sock;

        /* outgoing connection */
        tmpi = getaddrinfo(remoteHost, port, NULL, &ai);
        if (tmpi != 0) {
            fprintf(stderr, "%s:%s: %s\n", remoteHost, port, gai_strerror(tmpi));
            return 1;
        }
        SF(sock, socket, -1, (ai->ai_family, SOCK_STREAM, 0));
        SF(tmpi, connect, -1, (sock, ai->ai_addr, ai->ai_addrlen));
        freeaddrinfo(ai);
        pingpong(sock, cmd);

    }

    return 0;
}

ssize_t timeoutRecv(int sockfd, void *buf, size_t len, int flags, time_t sec, suseconds_t usec)
{
    struct timeval tv;
    ssize_t ret;
    int tmpi;

    /* set the timeout */
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    SF(tmpi, setsockopt, -1, (sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)));

    /* perform the op */
    ret = recv(sockfd, buf, len, flags);

    return ret;
}

ssize_t timeoutSend(int sockfd, const void *buf, size_t len, int flags, time_t sec, suseconds_t usec)
{
    struct timeval tv;
    ssize_t ret;
    int tmpi;

    /* set the timeout */
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    SF(tmpi, setsockopt, -1, (sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)));

    /* perform the op */
    ret = send(sockfd, buf, len, flags);

    return ret;
}

int ping(int sock)
{
    if (timeoutSend(sock, "I", 1, 0, sendTime, 0) != 1) {
        return 0;
    }
    return 1;
}

int pong(int sock)
{
    if (timeoutSend(sock, "O", 1, 0, sendTime, 0) != 1) {
        return 0;
    }
    return 1;
}

int get(int sock, time_t to, int *pings, int *pongs)
{
    char buf;
    if (timeoutRecv(sock, &buf, 1, 0, to, 0) != 1) {
        return 0;
    }
    if (buf == 'I') {
        (*pings)++;
    } else if (buf == 'O') {
        (*pongs)++;
    } else {
        return 0;
    }
    return 1;
}

void pingpong(int sock, char **cmd)
{
    int ipings, opongs, opings, ipongs;
    time_t cycletime, curtime;
    ipings = opongs = opings = ipongs = 0;

    while (1) {
        cycletime = time(NULL) + pingTime;

        /* ping */
        if (!ping(sock)) goto broken;
        opings++;

        /* wait for a pong */
        while (ipongs < opings) {
            if (!get(sock, pongTime, &ipings, &ipongs)) goto broken;
            while (ipings > opongs) {
                /* send a pong */
                if (!pong(sock)) goto broken;
                opongs++;
            }
        }

        /* then wait for any input */
        while (1) {
            curtime = time(NULL);
            if (curtime >= cycletime) break;
            get(sock, cycletime - curtime, &ipings, &ipongs);
            while (ipings > opongs) {
                if (!pong(sock)) goto broken;
                opongs++;
            }
        }

        ipings = opongs = opings = ipongs = 0;
    }

broken:
    /* our connection was lost, things are bad! */
    close(sock);
    if (cmd) {
        execvp(cmd[0], cmd);
        fprintf(stderr, "execvp failed.\n");
    } else {
        fprintf(stderr, "Connection lost.\n");
        exit(0);
    }
}
