/*
 * 05_zombie.c  —  ⑬ゾンビプロセス
 * ------------------------------------------------------------
 * 子が終了しても親が wait() しないと、カーネルは終了ステータスを
 * 保持するために task_struct を残す。これが「ゾンビ (Z / defunct)」。
 *
 * このプログラムは意図的にゾンビを作り、10 秒間保持する。
 * 別ターミナルで観察:
 *   ps -el | grep Z
 *   ps -o pid,ppid,stat,cmd --ppid <このPID>
 * STAT 欄が Z、CMD が <defunct> になる。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("parent PID=%d\n", (int)getpid());

    pid_t pid = xfork();

    if (pid == 0) {
        /* 子: すぐ終了する。親が回収しないのでゾンビになる。 */
        printf("  child PID=%d exiting now (親は wait しない)\n",
               (int)getpid());
        _exit(0);
    }

    /* 親: わざと wait() せずに寝る → 子はゾンビのまま */
    printf("parent NOT calling wait(). 別ターミナルで:\n");
    printf("  ps -o pid,ppid,stat,cmd --ppid %d\n", (int)getpid());
    printf("を実行すると STAT=Z / <defunct> が見える。10 秒待機...\n");
    fflush(stdout);
    sleep(10);

    printf("parent exits now → ゾンビは init/systemd に引き取られ回収される\n");
    return 0;
}
