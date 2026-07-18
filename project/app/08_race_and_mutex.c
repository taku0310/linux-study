/*
 * 08_race_and_mutex.c  —  ④共有メモリの罠 / スレッド同期
 * ------------------------------------------------------------
 * 「共有メモリは便利だが危険」を体感するサンプル。
 * 同じカウンタを多数スレッドで大量に ++ する。
 *   - RACE=1 (mutex なし): 結果が期待値より小さくなる(データ競合)
 *   - RACE=0 (mutex あり): 常に正しい
 * ビルド時マクロ USE_MUTEX で切り替える。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define N_THREADS   8
#define N_ITERS     1000000

static long g_counter = 0;
#ifdef USE_MUTEX
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < N_ITERS; i++) {
#ifdef USE_MUTEX
        pthread_mutex_lock(&g_lock);
        g_counter++;                 /* 臨界区間 */
        pthread_mutex_unlock(&g_lock);
#else
        g_counter++;                 /* 非アトミック: read-modify-write が競合 */
#endif
    }
    return NULL;
}

int main(void)
{
    pthread_t th[N_THREADS];

    for (int i = 0; i < N_THREADS; i++)
        xpthread_create(&th[i], worker, NULL);
    for (int i = 0; i < N_THREADS; i++)
        pthread_join(th[i], NULL);

    long expected = (long)N_THREADS * N_ITERS;
#ifdef USE_MUTEX
    printf("[mutex ] counter=%ld expected=%ld  %s\n",
           g_counter, expected, g_counter == expected ? "OK" : "MISMATCH");
#else
    printf("[race  ] counter=%ld expected=%ld  %s (差=%ld)\n",
           g_counter, expected,
           g_counter == expected ? "OK(たまたま)" : "DATA RACE",
           expected - g_counter);
#endif
    return 0;
}
