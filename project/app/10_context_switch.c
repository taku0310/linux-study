/*
 * 10_context_switch.c  —  ⑩コンテキストスイッチ / ⑪スケジューラ(CFS)
 * ------------------------------------------------------------
 * CPU 数より多いスレッドを走らせ、CFS がタイムスライスで
 * 各スレッドを切り替える様子を「自発的/非自発的コンテキスト
 * スイッチ回数」で観察する。
 *
 * 実行後に別ターミナルで:
 *   cat /proc/<PID>/status | grep -i ctxt
 *   pidstat -w -p <PID> 1     # voluntary/nonvoluntary switch/秒
 * を見ると切り替え回数が増えていく。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define N_THREADS 4
#define RUN_SEC   5

static volatile int g_stop = 0;

static void *cpu_burner(void *arg)
{
    long id = (long)arg;
    unsigned long loops = 0;
    while (!g_stop) {
        /* CPU を消費する。CFS が公平に時間を配分するため、
         * 各スレッドの loops はおおむね近い値になる。 */
        for (volatile int i = 0; i < 100000; i++) { }
        loops++;
    }
    printf("thread #%ld (TID=%d) loops=%lu\n",
           id, (int)get_tid(), loops);
    return NULL;
}

int main(void)
{
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    printf("online CPUs = %ld, threads = %d (=> 競合してコンテキストスイッチ発生)\n",
           ncpu, N_THREADS);
    printf("observe: cat /proc/%d/status | grep ctxt\n", (int)getpid());
    fflush(stdout);

    pthread_t th[N_THREADS];
    for (long i = 0; i < N_THREADS; i++)
        xpthread_create(&th[i], cpu_burner, (void *)i);

    sleep(RUN_SEC);
    g_stop = 1;

    for (int i = 0; i < N_THREADS; i++)
        pthread_join(th[i], NULL);

    printf("done. %d秒間で各スレッドがどれだけ回れたか比較してみよう。\n",
           RUN_SEC);
    return 0;
}
