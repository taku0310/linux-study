/*
 * 06_orphan.c  —  ⑭孤児プロセス
 * ------------------------------------------------------------
 * 親が先に終了し、子が生き残ると「孤児 (orphan)」になる。
 * 孤児は即座に init/systemd (PID 1)、または subreaper に
 * 「再親付け (re-parenting)」される。ゾンビとは別物で、
 * 孤児は正常に動き続け、終了時は新しい親が回収する。
 *
 * 子の PPID が途中で 1 (または subreaper の PID) に変わる様子を確認する。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = xfork();

    if (pid == 0) {
        /* ---- 子 ---- */
        printf("  child PID=%d, PPID=%d (最初の親)\n",
               (int)getpid(), (int)getppid());
        fflush(stdout);

        sleep(2);   /* この間に親が死ぬ */

        /* 親はもういない → PPID が 1 (systemd) 等に変わっている */
        printf("  child PID=%d, PPID=%d (親が死んだ後 → 再親付け)\n",
               (int)getpid(), (int)getppid());
        fflush(stdout);   /* _exit() は stdio を flush しないので明示的に */
        _exit(0);
    }

    /* ---- 親: 子より先に終了する ---- */
    printf("parent PID=%d exits immediately, leaving child orphaned\n",
           (int)getpid());
    return 0;
}
