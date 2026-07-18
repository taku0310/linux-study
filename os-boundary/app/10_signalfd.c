/*
 * 10_signalfd.c  —  シグナル(3): signalfd で「シグナルもFDに」
 * ------------------------------------------------------------
 * 集大成。従来のシグナルは「非同期ハンドラ」で扱うため、
 * async-signal-safe 制約が厳しく、イベントループと相性が悪い。
 *
 * Linux の signalfd はシグナルを「FD から read できる普通のイベント」
 * に変換する。これで epoll/poll のループに一元化でき、
 *   ・ソケット/パイプ/タイマ/シグナル を全部同じ epoll で待つ
 * という、実務のイベント駆動設計そのものになる。
 *
 * 手順:
 *   1) 対象シグナルを sigprocmask でブロック(通常配送を止める)
 *   2) signalfd() でその集合を FD 化
 *   3) epoll で待ち、read で signalfd_siginfo を取り出す
 *
 * 実行後、Ctrl-C か  kill -TERM <PID>  で終了。
 * ------------------------------------------------------------
 */
#define _GNU_SOURCE
#include "io_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>

int main(void)
{
    /* 1) SIGINT/SIGTERM を「通常配送」からブロックする。
     *    こうしないと signalfd ではなく従来ハンドラ/既定動作に行ってしまう。 */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0)
        die("sigprocmask");

    /* 2) その集合を FD にする。以後シグナルはここから read できる。 */
    int sfd = signalfd(-1, &mask, 0);
    if (sfd < 0) die("signalfd");

    /* 3) epoll に signalfd を登録(他の FD と横並びに扱える)。 */
    int epfd = epoll_create1(0);
    if (epfd < 0) die("epoll_create1");
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = sfd };
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev) < 0)
        die("epoll_ctl");

    printf("PID=%d イベントループ実行中。Ctrl-C か kill -TERM %d で終了。\n",
           (int)getpid(), (int)getpid());

    struct epoll_event events[4];
    int running = 1;
    while (running) {
        int n = epoll_wait(epfd, events, 4, -1);
        if (n < 0) die("epoll_wait");

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == sfd) {
                struct signalfd_siginfo si;
                if (read(sfd, &si, sizeof(si)) != sizeof(si))
                    die("read signalfd");
                /* ここは通常のメインループ内。printf も安全に使える!
                 * (ハンドラではないので async-signal-safe 制約が無い) */
                printf("signalfd 経由でシグナル %u を受信 → 終了\n",
                       si.ssi_signo);
                running = 0;
            }
        }
    }

    close(sfd);
    close(epfd);
    printf("クリーンに終了しました\n");
    return 0;
}
