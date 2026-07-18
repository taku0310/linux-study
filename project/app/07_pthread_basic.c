/*
 * 07_pthread_basic.c  —  ②スレッド / ⑧pthread_create / ⑨pthread_join / ④メモリ空間
 * ------------------------------------------------------------
 * 同一プロセス内に複数スレッドを作る。全スレッドが
 *   - 同じ PID を共有する
 *   - 異なる TID (LWP) を持つ
 *   - 同じグローバル変数を共有する (→ 競合の原因)
 * ことを確認する。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define N_THREADS 3

/* 全スレッドで共有されるグローバル変数 (.data セグメント) */
static int g_shared = 0;

static void *worker(void *arg)
{
    long id = (long)arg;
    print_ids("worker");   /* PID は main と同じ、TID は各自異なる */

    /* 排他なしで共有変数を更新 → 本来はデータ競合。
     * デモとして「共有されている」事実を示す。 */
    g_shared += 1;
    printf("  thread #%ld incremented g_shared -> %d\n", id, g_shared);
    return (void *)(id * 10);   /* 戻り値は join で受け取れる */
}

int main(void)
{
    pthread_t th[N_THREADS];

    print_ids("main");

    for (long i = 0; i < N_THREADS; i++) {
        xpthread_create(&th[i], worker, (void *)i);
    }

    /* join でスレッド終了を待ち、戻り値を回収する。
     * join しないとリソースリーク(detach しない限り)。 */
    for (int i = 0; i < N_THREADS; i++) {
        void *ret;
        pthread_join(th[i], &ret);
        printf("  joined thread #%d, return=%ld\n", i, (long)ret);
    }

    printf("final g_shared = %d (期待値 %d)\n", g_shared, N_THREADS);
    return 0;
}
