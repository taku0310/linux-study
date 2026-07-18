/*
 * 07_epoll.c  —  I/O多重化(3): epoll(Linux固有・本命)
 * ------------------------------------------------------------
 * select/poll は「毎回全FDをカーネルに渡して O(N) 走査」する。
 * 監視FDが数千〜数万になるとこれが重い。
 *
 * epoll はカーネル側に「監視対象の集合」を1度登録しておき、
 * epoll_wait は「実際に起きたイベントだけ」を返す(O(1)に近い)。
 * 高性能サーバ・ROS2 の下回り・イベントループの標準。
 *
 * 使い方:
 *   epoll_create1() で epoll インスタンス(これも FD)を作る
 *   epoll_ctl(ADD)  で監視対象を登録
 *   epoll_wait()    で発生イベントを取得
 *
 * デモ構成は 05/06 と同じ(2パイプ + 子が時間差で書く)。
 * ------------------------------------------------------------
 */
#define _GNU_SOURCE
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
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

    int epfd = epoll_create1(0);
    if (epfd < 0) die("epoll_create1");

    /* 監視対象をカーネルに登録。ev.data に任意の目印を持たせられる
     * (ここでは fd 自体を入れておき、後で識別に使う)。 */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = p0[0];
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, p0[0], &ev) < 0) die("epoll_ctl");
    ev.data.fd = p1[0];
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, p1[0], &ev) < 0) die("epoll_ctl");

    int open_count = 2;
    struct epoll_event events[8];

    while (open_count > 0) {
        /* 「起きたイベントだけ」が events[] に返る。-1 = 無限待ち。 */
        int nready = epoll_wait(epfd, events, 8, -1);
        if (nready < 0) die("epoll_wait");
        printf("epoll_wait(): %d 個のイベント発生\n", nready);

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;
            char buf[64];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n <= 0) {
                printf("  fd %d: EOF、epoll から削除\n", fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                open_count--;
            } else {
                buf[n] = '\0';
                printf("  fd %d から受信: \"%s\"\n", fd, buf);
            }
        }
    }

    close(epfd);
    wait(NULL);
    printf("両パイプ完了。epoll ループ終了。\n");
    return 0;
}
