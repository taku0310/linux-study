/*
 * 04_nonblock.c  —  ブロッキング vs ノンブロッキング I/O
 * ------------------------------------------------------------
 * デフォルトの read() は「データが来るまで待つ(ブロックする)」。
 * O_NONBLOCK を立てると、データが無いとき即座に -1 / errno=EAGAIN
 * を返して制御を戻す。これがイベント駆動(epoll 等)の前提。
 *
 * パイプを作り、まだ何も書いていない読み口を
 *   (1) ブロッキングで read → 待たされる(のでスキップして説明のみ)
 *   (2) ノンブロッキングで read → EAGAIN で即戻る
 * を確認する。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(void)
{
    int pfd[2];
    if (pipe(pfd) < 0)          /* pfd[0]=読み口, pfd[1]=書き口 */
        die("pipe");

    /* 読み口をノンブロッキングに設定。 */
    set_nonblock(pfd[0]);

    char buf[64];

    /* まだ誰も書いていない → ブロッキングなら永久に待つところ。
     * ノンブロッキングなので EAGAIN で即戻る。 */
    ssize_t n = read(pfd[0], buf, sizeof(buf));
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        printf("data無し: read() が即 -1 / errno=EAGAIN で戻った(待たない)\n");
    } else {
        printf("予期しない結果: n=%zd\n", n);
    }

    /* 書き口に書いてから読むと、今度はちゃんと読める。 */
    const char *msg = "now there is data";
    write_all(pfd[1], msg, strlen(msg));

    n = read(pfd[0], buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("書き込み後の read(): %zd バイト \"%s\"\n", n, buf);
    }

    close(pfd[0]);
    close(pfd[1]);
    return 0;
}
