/*
 * 04_wait.c  —  ⑦wait() / 終了ステータスの読み方
 * ------------------------------------------------------------
 * 複数の子を作り、それぞれ異なる終了コードで終わらせる。
 * 親は wait() で 1 つずつ回収し、
 *   WIFEXITED / WEXITSTATUS / WIFSIGNALED / WTERMSIG
 * マクロで終了理由を判定する。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define N_CHILDREN 3

int main(void)
{
    print_ids("parent");

    for (int i = 0; i < N_CHILDREN; i++) {
        pid_t pid = xfork();
        if (pid == 0) {
            /* 子: i 秒だけ働くふりをして、終了コード (10+i) で終わる */
            printf("  child #%d (PID=%d) working %d s ...\n",
                   i, (int)getpid(), i + 1);
            fflush(stdout);
            sleep(i + 1);
            _exit(10 + i);
        }
    }

    /* 親: 子の数だけ回収する。終わった順に返ってくる。 */
    printf("parent waiting for %d children ...\n", N_CHILDREN);
    for (int i = 0; i < N_CHILDREN; i++) {
        wait_and_report();
    }

    printf("all children reaped.\n");
    return 0;
}
