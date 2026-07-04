/*
 * main_demo.c  —  app / control / driver の 3 層を通しで使う例
 * ------------------------------------------------------------
 * 実務での典型形:
 *   1) 親プロセスが子ワーカープロセスを fork
 *   2) 子は driver 層経由でデバイスを読む
 *   3) 親は wait で結果を回収
 * さらに親プロセス内では pthread で並行タスクも回す。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* driver 層の関数 (driver/driver_stub.c で定義) */
int driver_read_demo(size_t n);

static void *bg_task(void *arg)
{
    (void)arg;
    print_ids("bg-thread");
    return NULL;
}

int main(void)
{
    printf("=== main_demo: process(fork) + thread + driver ===\n");
    print_ids("main");

    /* (A) バックグラウンドスレッドを起動 */
    pthread_t th;
    xpthread_create(&th, bg_task, NULL);

    /* (B) ワーカープロセスを fork してデバイス読み込みを担当させる */
    pid_t pid = xfork();
    if (pid == 0) {
        print_ids("worker-proc");
        driver_read_demo(16);
        _exit(0);
    }

    /* (C) 親はスレッドとプロセス両方を回収 */
    pthread_join(th, NULL);
    wait_and_report();

    printf("=== all done ===\n");
    return 0;
}
