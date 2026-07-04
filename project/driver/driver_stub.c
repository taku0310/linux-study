/*
 * driver_stub.c  —  driver 層のプレースホルダ
 * ------------------------------------------------------------
 * このシリーズの最終目標は「デバイスドライバ開発」。
 * 本物のドライバはカーネル空間で動く .ko だが、学習初期は
 * ユーザ空間から擬似的にデバイスを叩く層をここに置く。
 *
 * ここでは「デバイスを読む」処理を、実在する擬似デバイス
 * /dev/urandom の read で代用する。app/control 層から見た
 * インタフェース(初期化→read→クローズ)の形を体験するのが目的。
 * ------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* 擬似デバイスから n バイト読み、先頭を 16 進表示する。 */
int driver_read_demo(size_t n)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/urandom");
        return -1;
    }

    unsigned char *buf = malloc(n);
    if (!buf) {
        close(fd);
        return -1;
    }

    ssize_t got = read(fd, buf, n);
    if (got < 0) {
        perror("read");
        free(buf);
        close(fd);
        return -1;
    }

    printf("[driver] read %zd bytes from /dev/urandom: ", got);
    for (ssize_t i = 0; i < got && i < 8; i++)
        printf("%02x ", buf[i]);
    printf("...\n");

    free(buf);
    close(fd);
    return 0;
}

#ifdef DRIVER_STANDALONE
int main(void)
{
    return driver_read_demo(16) == 0 ? 0 : 1;
}
#endif
