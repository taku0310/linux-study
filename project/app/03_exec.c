/*
 * 03_exec.c  —  ⑥exec()
 * ------------------------------------------------------------
 * fork() + exec() は Unix の「新しいプログラムを起動する」定石。
 *   fork()  : プロセスを複製する(器を作る)
 *   exec()  : そのプロセスのアドレス空間を別プログラムで上書きする
 * exec が成功すると元のコード(この main の続き)には戻らない。
 * ------------------------------------------------------------
 */
#include "proc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    printf("=== fork + exec: 子で /bin/ls -l を実行 ===\n");
    print_ids("parent");

    pid_t pid = xfork();

    if (pid == 0) {
        /* ---- 子プロセス: 自分自身を ls に化けさせる ---- */
        print_ids("child-before-exec");

        /* execlp: PATH を検索し、引数リストを渡す。
         * 末尾は必ず (char *)NULL で終端する。 */
        execlp("ls", "ls", "-l", "--color=never", (char *)NULL);

        /* ここに到達 = exec 失敗 (プログラムが見つからない等) */
        perror("execlp");
        _exit(127);   /* シェルの「コマンド無し」慣習に合わせる */
    } else {
        /* ---- 親プロセス ---- */
        wait_and_report();
        printf("=== 親は生き残っている ===\n");
    }

    return 0;
}
