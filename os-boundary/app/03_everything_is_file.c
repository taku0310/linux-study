/*
 * 03_everything_is_file.c  —  "everything is a file" と lseek
 * ------------------------------------------------------------
 * Unix 思想: 通常ファイル・デバイス・カーネル情報(/proc)まで、
 * すべて「FD に対する read/write」という同じ API で扱える。
 *
 * 前半: 同じ read() で /proc/self/status(カーネルが生成する
 *        仮想ファイル)とディスク上のファイルを読む。
 * 後半: lseek でファイル内の読み書き位置(オフセット)を動かす。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void dump_head(const char *path, size_t max)
{
    int fd = xopen(path, O_RDONLY, 0);
    char buf[256];
    ssize_t n = read_all(fd, buf, (max < sizeof(buf) - 1) ? max : sizeof(buf) - 1);
    if (n < 0)
        die("read");
    buf[n] = '\0';
    printf("---- %s (先頭 %zd バイト) ----\n%s\n", path, n, buf);
    close(fd);
}

int main(void)
{
    /* /proc/self/... はディスクに実体が無い。カーネルが read の
     * たびに内容を「生成」する仮想ファイル。だが read() は通常ファイルと同じ。 */
    dump_head("/proc/self/status", 160);

    /* lseek のデモ: 一時ファイルに 0-9 を書き、位置を戻して部分読み。 */
    const char *path = "/tmp/os_boundary_lseek.txt";
    int fd = xopen(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    write_all(fd, "0123456789", 10);

    /* SEEK_SET: 先頭から 3 バイト目へ移動。戻り値は新しいオフセット。 */
    off_t pos = lseek(fd, 3, SEEK_SET);
    printf("lseek(fd, 3, SEEK_SET) -> offset %ld\n", (long)pos);

    char c[4];
    ssize_t n = read_all(fd, c, 3);   /* オフセット3から3バイト = "345" */
    c[n] = '\0';
    printf("read 3 bytes from offset 3: \"%s\"\n", c);

    /* SEEK_END で末尾+オフセットも取れる(ファイルサイズ確認の常套手段)。 */
    off_t size = lseek(fd, 0, SEEK_END);
    printf("file size (lseek SEEK_END) = %ld bytes\n", (long)size);

    close(fd);
    unlink(path);
    return 0;
}
