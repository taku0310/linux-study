/*
 * io_utils.c  —  io_utils.h の実装
 * ------------------------------------------------------------
 * システムコールの「戻り値 + errno」規約を一箇所で処理する。
 * ------------------------------------------------------------
 */
#include "io_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void die(const char *msg)
{
    /* perror は "msg: <errno に対応する説明>" を stderr に出す。 */
    perror(msg);
    exit(EXIT_FAILURE);
}

int xopen(const char *path, int flags, mode_t mode)
{
    int fd = open(path, flags, mode);
    if (fd < 0)
        die(path);          /* どのファイルで失敗したか分かるよう path を渡す */
    return fd;
}

ssize_t read_all(int fd, void *buf, size_t n)
{
    size_t done = 0;
    char *p = buf;

    while (done < n) {
        ssize_t r = read(fd, p + done, n - done);
        if (r < 0) {
            if (errno == EINTR)
                continue;    /* シグナルで中断された → やり直す */
            return -1;       /* 本物のエラー */
        }
        if (r == 0)
            break;           /* EOF: これ以上読めない */
        done += (size_t)r;
    }
    return (ssize_t)done;
}

ssize_t write_all(int fd, const void *buf, size_t n)
{
    size_t done = 0;
    const char *p = buf;

    while (done < n) {
        ssize_t w = write(fd, p + done, n - done);
        if (w < 0) {
            if (errno == EINTR)
                continue;    /* シグナルで中断された → やり直す */
            return -1;
        }
        /* write は「短い書き込み」があり得る(パイプ/ソケット等)。
         * 全部書けるまでループするのが定石。 */
        done += (size_t)w;
    }
    return (ssize_t)done;
}

void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);   /* 現在のフラグを取得 */
    if (flags < 0)
        die("fcntl(F_GETFL)");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)   /* O_NONBLOCK を追加 */
        die("fcntl(F_SETFL)");
}

void install_handler(int signo, void (*handler)(int), int flags)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);   /* ハンドラ実行中に追加ブロックするシグナル無し */
    sa.sa_flags = flags;
    if (sigaction(signo, &sa, NULL) < 0)
        die("sigaction");
}
