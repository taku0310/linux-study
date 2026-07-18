/*
 * dev_read.c  —  driver 層プレースホルダ
 * ------------------------------------------------------------
 * 「everything is a file」の実例として、擬似デバイスを FD 経由で
 * 読むユーザ空間ドライバ層の最小形。前回モジュールの driver_stub と
 * 同じ役割で、app/control 層から見た「デバイス読み取り」の形を示す。
 *
 * ここでは実在の擬似デバイス /dev/zero(読むと 0 が無限に出る)を
 * 使い、通常ファイルと全く同じ open/read/close で扱えることを見せる。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/* /dev/zero から n バイト読み、全て 0 だったか検証する。 */
int dev_read_demo(size_t n)
{
    int fd = xopen("/dev/zero", O_RDONLY, 0);

    unsigned char buf[64];
    size_t remain = n;
    int all_zero = 1;
    while (remain > 0) {
        size_t chunk = remain < sizeof(buf) ? remain : sizeof(buf);
        ssize_t r = read_all(fd, buf, chunk);
        if (r < 0) { close(fd); return -1; }
        for (ssize_t i = 0; i < r; i++)
            if (buf[i] != 0) all_zero = 0;
        remain -= (size_t)r;
    }
    close(fd);

    printf("[driver] /dev/zero から %zu バイト読了(all_zero=%s)\n",
           n, all_zero ? "true" : "false");
    return 0;
}

#ifdef DRIVER_STANDALONE
int main(void)
{
    return dev_read_demo(128) == 0 ? 0 : 1;
}
#endif
