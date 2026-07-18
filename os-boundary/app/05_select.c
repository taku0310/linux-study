/*
 * 05_select.c  —  I/O多重化(1): select()
 * ------------------------------------------------------------
 * 「複数の FD を1スレッドで同時に待つ」のが I/O多重化。
 * ブロッキング read を FD の数だけ並べると1つに張り付いてしまうが、
 * select は「どれか1つでも読める状態になるまで」まとめて待てる。
 *
 * デモ構成: 2本のパイプを作り、子プロセスが
 *   pipe0 に 1秒後、pipe1 に 2秒後に書き込む。
 * 親は select で両方を監視し、届いた順に表示する。
 *
 * select の弱点: 監視 FD 数が FD_SETSIZE(通常1024)まで、
 *              毎回 fd_set を作り直す、O(N) 走査。→ poll/epoll へ。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/wait.h>

/* 子: pipe[i][1] へ delay 秒後に書く。 */
static void child_writer(int w0, int w1)
{
    sleep(1);
    write_all(w0, "pipe0: 1秒後に到着", 25);
    close(w0);
    sleep(1);
    write_all(w1, "pipe1: 2秒後に到着", 25);
    close(w1);
    _exit(0);
}

int main(void)
{
    int p0[2], p1[2];
    if (pipe(p0) < 0 || pipe(p1) < 0)
        die("pipe");

    pid_t pid = fork();
    if (pid < 0) die("fork");
    if (pid == 0) {
        close(p0[0]); close(p1[0]);      /* 子は読み口不要 */
        child_writer(p0[1], p1[1]);
    }
    close(p0[1]); close(p1[1]);          /* 親は書き口不要 */

    int r0 = p0[0], r1 = p1[0];
    int open_count = 2;

    while (open_count > 0) {
        fd_set rset;
        FD_ZERO(&rset);                  /* 毎回作り直すのが select の作法 */
        if (r0 >= 0) FD_SET(r0, &rset);
        if (r1 >= 0) FD_SET(r1, &rset);
        int maxfd = (r0 > r1 ? r0 : r1);

        /* nfds = 最大FD+1。タイムアウト NULL = 無限待ち。 */
        int ready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (ready < 0) die("select");
        printf("select(): %d 個の FD が読み取り可能\n", ready);

        int fds[2] = { r0, r1 };
        for (int i = 0; i < 2; i++) {
            int fd = fds[i];
            if (fd >= 0 && FD_ISSET(fd, &rset)) {
                char buf[64];
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                if (n <= 0) {            /* 0 = EOF(書き口が閉じた) */
                    printf("  fd %d: EOF、監視から外す\n", fd);
                    close(fd);
                    if (i == 0) r0 = -1; else r1 = -1;
                    open_count--;
                } else {
                    buf[n] = '\0';
                    printf("  fd %d から受信: \"%s\"\n", fd, buf);
                }
            }
        }
    }

    wait(NULL);
    printf("両パイプ完了。select ループ終了。\n");
    return 0;
}
