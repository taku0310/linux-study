/*
 * 02_fd_basic.c  —  ファイルディスクリプタと基本 I/O
 * ------------------------------------------------------------
 * open → write → close、そして read で読み戻す最小例。
 * FD(ファイルディスクリプタ)= プロセスの「開いているファイル表」
 * のインデックス(小さな非負整数)。
 *   0 = 標準入力 (STDIN_FILENO)
 *   1 = 標準出力 (STDOUT_FILENO)
 *   2 = 標準エラー (STDERR_FILENO)
 * open は「最小の未使用 FD番号」を返す。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define PATH "/tmp/os_boundary_fd_demo.txt"

int main(void)
{
    const char *msg = "hello, file descriptor!\n";

    /* O_CREAT|O_WRONLY|O_TRUNC で新規作成 or 切り詰めて書き込み。
     * 第3引数 0644 = 所有者rw / グループr / 他r のパーミッション。 */
    int fd = xopen(PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    printf("open(\"%s\") = fd %d (0,1,2 は標準ストリームで予約済み)\n",
           PATH, fd);

    if (write_all(fd, msg, strlen(msg)) < 0)
        die("write");
    close(fd);   /* 使い終わったら必ず閉じる(FDリーク防止) */

    /* 今度は読み戻す。 */
    fd = xopen(PATH, O_RDONLY, 0);
    char buf[128];
    ssize_t n = read_all(fd, buf, sizeof(buf) - 1);
    if (n < 0)
        die("read");
    buf[n] = '\0';
    printf("read back %zd bytes: %s", n, buf);
    close(fd);

    /* FD 1(標準出力)へ直接 write するのも同じ system call。 */
    const char *direct = "この行は write(1, ...) で直接出した\n";
    write_all(STDOUT_FILENO, direct, strlen(direct));

    unlink(PATH);   /* 後始末 */
    return 0;
}
