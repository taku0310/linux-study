/*
 * 02_fork.c  —  ⑤fork()
 * ------------------------------------------------------------
 * fork() は 1 回呼ぶと 2 回「戻る」システムコール。
 *   - 親には子の PID (>0) が返る
 *   - 子には 0 が返る
 *   - 失敗時は -1
 * 親子はコピーオンライト(COW)された独立のメモリ空間を持つ。
 * 変数を書き換えても互いに影響しないことを確認する。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int shared_value = 100;   /* fork 後は親子で別々の実体になる */

    printf("before fork: value=%d\n", shared_value);
    print_ids("parent-pre");

    pid_t pid = xfork();

    if (pid == 0) {
        /* ---- 子プロセス ---- */
        shared_value += 1;    /* COW により親には見えない */
        print_ids("child");
        printf("  child : value=%d (親からコピーした後 +1)\n", shared_value);
        _exit(0);             /* 子は _exit で即終了(atexit/バッファ二重flush回避) */
    } else {
        /* ---- 親プロセス ---- */
        shared_value += 1000;
        print_ids("parent");
        printf("  parent: value=%d\n", shared_value);
        wait_and_report();    /* 子を回収してゾンビ化を防ぐ */
    }

    return 0;
}
