/*
 * proc_utils.c
 * ------------------------------------------------------------
 * proc_utils.h の実装。システムコールの薄いラッパ群。
 * ------------------------------------------------------------
 */
#define _GNU_SOURCE      /* gettid() の宣言を有効化 */
#include "proc_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>

pid_t get_tid(void)
{
#ifdef SYS_gettid
    /* glibc が gettid() ラッパを持たない環境でも動くよう
     * syscall(SYS_gettid) を直接呼ぶ。 */
    return (pid_t)syscall(SYS_gettid);
#else
    return getpid();
#endif
}

void print_ids(const char *tag)
{
    /* getpid()   : プロセスID (スレッド共通)
     * getppid()  : 親プロセスID
     * get_tid()  : スレッドID (LWP)。シングルスレッドでは PID と一致 */
    printf("[%-14s] PID=%d PPID=%d TID=%d\n",
           tag, (int)getpid(), (int)getppid(), (int)get_tid());
    fflush(stdout);   /* fork 前の出力残留を避けるため明示 flush */
}

pid_t xfork(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    return pid;
}

void xpthread_create(pthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg)
{
    int rc = pthread_create(thread, NULL, start_routine, arg);
    if (rc != 0) {
        /* pthread 系は errno ではなく戻り値がエラーコード */
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        exit(EXIT_FAILURE);
    }
}

pid_t wait_and_report(void)
{
    int status;
    pid_t child = wait(&status);
    if (child < 0) {
        perror("wait");
        return -1;
    }

    if (WIFEXITED(status)) {
        printf("child PID=%d exited normally, status=%d\n",
               (int)child, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("child PID=%d killed by signal %d (%s)\n",
               (int)child, WTERMSIG(status), strsignal(WTERMSIG(status)));
    } else {
        printf("child PID=%d ended (unknown state)\n", (int)child);
    }
    fflush(stdout);
    return child;
}
