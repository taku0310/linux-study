/*
 * 01_process_basic.c  —  ①プロセスとは / ⑫PID・TID
 * ------------------------------------------------------------
 * 単一プロセスが自分の ID を表示するだけの最小例。
 * シングルスレッドでは PID == TID になることを確認する。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("=== single process ===\n");
    print_ids("main");

    /* このプロセスが生きている間に別ターミナルで
     *   ps -o pid,ppid,stat,cmd -p <PID>
     * を実行して観察できるよう、少し待つ。 */
    printf("sleeping 3s ... (別ターミナルで ps/top で観察可能)\n");
    fflush(stdout);
    sleep(3);

    printf("done\n");
    return 0;
}
