/*
 * 09_pid_tid.c  —  ⑫PID・TID の関係を厳密に見る
 * ------------------------------------------------------------
 * Linux ではスレッドも「タスク(task_struct)」として実装される。
 *   - getpid()        : TGID (スレッドグループID) = 一般に言う PID
 *   - gettid()        : PID (カーネル内部の task の ID) = 一般に言う TID
 * 用語がねじれているので注意:
 *   ユーザ空間の "PID"  == カーネルの TGID
 *   ユーザ空間の "TID"  == カーネルの pid
 *
 * /proc/<pid>/task/ 以下に各スレッドが並ぶことも確認できる。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

static void *worker(void *arg)
{
    long id = (long)arg;
    printf("thread #%ld: getpid()=%d  gettid()=%d  %s\n",
           id, (int)getpid(), (int)get_tid(),
           (getpid() == get_tid()) ? "(=main thread)" : "(worker LWP)");
    fflush(stdout);
    sleep(5);   /* /proc 観察用に生かしておく */
    return NULL;
}

int main(void)
{
    printf("main: getpid()=%d gettid()=%d\n",
           (int)getpid(), (int)get_tid());
    printf("観察: 別ターミナルで  ls /proc/%d/task/  や  ps -T -p %d\n",
           (int)getpid(), (int)getpid());
    fflush(stdout);

    pthread_t th[2];
    for (long i = 0; i < 2; i++)
        xpthread_create(&th[i], worker, (void *)(i + 1));
    for (int i = 0; i < 2; i++)
        pthread_join(th[i], NULL);

    return 0;
}
