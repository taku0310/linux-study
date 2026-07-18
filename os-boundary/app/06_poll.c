/*
 * 06_poll.c  —  I/O多重化(2): poll()
 * ------------------------------------------------------------
 * select と同じ目的だが、
 *   - FD_SETSIZE(1024)の上限が無い
 *   - 監視対象を pollfd 配列で表現(fd_set の作り直し不要)
 * という改良版。監視FDごとに events(監視したいイベント)を指定し、
 * revents(実際に起きたイベント)を受け取る。
 *
 * デモ構成は 05_select.c と同じ(2パイプ + 子が時間差で書く)。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>

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
        close(p0[0]); close(p1[0]);
        child_writer(p0[1], p1[1]);
    }
    close(p0[1]); close(p1[1]);

    /* pollfd 配列で監視対象を宣言。POLLIN = 読み取り可能を監視。 */
    struct pollfd fds[2];
    fds[0].fd = p0[0]; fds[0].events = POLLIN;
    fds[1].fd = p1[0]; fds[1].events = POLLIN;
    int open_count = 2;

    while (open_count > 0) {
        /* 第2引数 = 配列要素数、第3引数 = タイムアウトms(-1 = 無限)。 */
        int ready = poll(fds, 2, -1);
        if (ready < 0) die("poll");
        printf("poll(): %d 個のイベント\n", ready);

        for (int i = 0; i < 2; i++) {
            if (fds[i].fd < 0) continue;
            /* revents に POLLIN(データ)か POLLHUP(相手が閉じた)が立つ。 */
            if (fds[i].revents & (POLLIN | POLLHUP)) {
                char buf[64];
                ssize_t n = read(fds[i].fd, buf, sizeof(buf) - 1);
                if (n <= 0) {
                    printf("  fd %d: EOF、監視から外す\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;      /* fd を負にすると poll は無視する */
                    open_count--;
                } else {
                    buf[n] = '\0';
                    printf("  fd %d から受信: \"%s\"\n", fds[i].fd, buf);
                }
            }
        }
    }

    wait(NULL);
    printf("両パイプ完了。poll ループ終了。\n");
    return 0;
}
