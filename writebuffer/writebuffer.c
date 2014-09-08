#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "helpers.h"

#define BUFSZ (16*1024*1024)
#define MAXBUFS 128

#define BUF_TYPE_NORMAL     0
#define BUF_TYPE_END        1
#define BUF_TYPE_HEAD       2
#define BUF_TYPE_TAIL       3

typedef struct Buffer_ {
    struct Buffer_ *next;
    pthread_mutex_t lock;
    unsigned char type;
    unsigned long length;
    unsigned char *buf;
} Buffer;

/* the in buffer */
static Buffer inBuffer, *inBufferTail;
static pthread_mutex_t inBufferTailLock = PTHREAD_MUTEX_INITIALIZER;

/* and the out buffer */
static Buffer outBuffer, *outBufferTail;
static pthread_mutex_t outBufferTailLock = PTHREAD_MUTEX_INITIALIZER;
static sem_t outBufferSem;

/* and our buffer count */
static unsigned long bufCt = 0;

/* the writer thread */
static void *writer(void *ignore)
{
    Buffer *cur, *tail;

    while (1) {
        sem_wait(&outBufferSem);

        /* lock the first item */
        pthread_mutex_lock(&outBuffer.lock);
        cur = outBuffer.next;
        pthread_mutex_lock(&cur->lock);

        /* remove it from the list */
        outBuffer.next = cur->next;
        pthread_mutex_unlock(&outBuffer.lock);

        /* write it out */
        if (cur->type == BUF_TYPE_NORMAL) {
            write(1, cur->buf, BUFSZ);
        } else if (cur->type == BUF_TYPE_END) {
            write(1, cur->buf, cur->length);
            pthread_mutex_unlock(&cur->lock);
            free(cur->buf);
            free(cur);
            break;
        }

        /* return it to the bufferstream */
        pthread_mutex_lock(&inBufferTailLock);
        tail = inBufferTail;
        pthread_mutex_lock(&tail->lock);
        tail->buf = cur->buf;
        tail->type = BUF_TYPE_NORMAL;
        tail->next = cur;
        cur->next = NULL;
        cur->type = BUF_TYPE_TAIL;
        cur->buf = NULL;
        inBufferTail = cur;
        pthread_mutex_unlock(&cur->lock);
        pthread_mutex_unlock(&tail->lock);
        pthread_mutex_unlock(&inBufferTailLock);
    }

    return NULL;
}

/* create a new buffer or get it from inBuffers */
static Buffer *newBuffer()
{
    static Buffer *cur;
    pthread_mutex_lock(&inBuffer.lock);
    cur = inBuffer.next;
    pthread_mutex_lock(&cur->lock);
    if (cur->type == BUF_TYPE_NORMAL) {
        /* ready for writing! */
        inBuffer.next = cur->next;
        pthread_mutex_unlock(&cur->lock);
        pthread_mutex_unlock(&inBuffer.lock);
        cur->length = 0;
        return cur;
    }
    pthread_mutex_unlock(&cur->lock);
    pthread_mutex_unlock(&inBuffer.lock);

    /* we need to allocate a new one */
    /* FIXME: shouldn't need/use SF here */
    SF(cur, malloc, NULL, (sizeof(Buffer)));
    SF(cur->buf, malloc, NULL, (BUFSZ));
    pthread_mutex_init(&cur->lock, NULL);
    cur->type = BUF_TYPE_NORMAL;
    cur->length = 0;
    return cur;
}

int main()
{
    int tmpi;
    pthread_t writerTh;
    ssize_t rd;
    Buffer *cur, *tail;

    inBuffer.type = outBuffer.type = BUF_TYPE_HEAD;

    sem_init(&outBufferSem, 0, 0);

    /* make our in and out buffer tails */
    pthread_mutex_init(&inBuffer.lock, NULL);
    SF(inBufferTail, malloc, NULL, (sizeof(Buffer)));
    inBuffer.next = inBufferTail;
    pthread_mutex_init(&inBufferTail->lock, NULL);
    inBufferTail->type = BUF_TYPE_TAIL;
    pthread_mutex_init(&outBuffer.lock, NULL);
    SF(outBufferTail, malloc, NULL, (sizeof(Buffer)));
    outBuffer.next = outBufferTail;
    pthread_mutex_init(&outBufferTail->lock, NULL);
    outBufferTail->type = BUF_TYPE_TAIL;

    /* start our writing thread */
    tmpi = pthread_create(&writerTh, NULL, writer, NULL);
    if (tmpi != 0) {
        fprintf(stderr, "Error creating writer thread!\n");
        exit(1);
    }

    /* get a fresh buffer */
    cur = newBuffer();

    /* and perform the reading */
    while ((rd = read(0, cur->buf + cur->length, BUFSZ - cur->length)) >= 0) {
        cur->length += rd;
        if (cur->length == BUFSZ || rd == 0) {
            /* read all we can for now, add it to the write queue */
            pthread_mutex_lock(&outBufferTailLock);
            tail = outBufferTail;
            pthread_mutex_lock(&tail->lock);

            tail->next = cur;
            tail->type = (rd == 0) ? BUF_TYPE_END : BUF_TYPE_NORMAL;
            tail->length = cur->length;
            tail->buf = cur->buf;

            cur->next = NULL;
            cur->type = BUF_TYPE_TAIL;
            cur->buf = NULL;

            outBufferTail = cur;

            pthread_mutex_unlock(&tail->lock);
            pthread_mutex_unlock(&outBufferTailLock);
            sem_post(&outBufferSem);

            cur = newBuffer();
        }

        if (rd == 0) break;
    }

    pthread_join(writerTh, NULL);

    return 0;
}
