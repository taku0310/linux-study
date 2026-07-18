/*
 * 01_errno.c  —  エラーと errno / 戻り値規約
 * ------------------------------------------------------------
 * システムコールの失敗の伝え方を理解する。
 *   - 多くは「失敗時 -1 を返し、errno に理由をセット」する規約。
 *   - errno はスレッドごとに独立(TLS)。呼び出し直後に読むこと。
 *   - perror() / strerror() で人間可読な文字列にできる。
 * ------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>   /* strerror */
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    /* わざと存在しないファイルを開いて失敗させる。 */
    int fd = open("/no/such/file", O_RDONLY);
    if (fd < 0) {
        /* 3通りの表示方法を比較する。 */
        int e = errno;                       /* すぐ退避(後続の関数が errno を壊す) */

        printf("open() returned %d\n", fd);
        printf("errno         = %d\n", e);
        printf("strerror(e)   = %s\n", strerror(e));   /* 文字列を得る */
        perror("open /no/such/file");                  /* "...: 説明" を stderr へ */

        /* ENOENT (No such file or directory) を名前で判定する例。 */
        if (e == ENOENT)
            printf("→ ファイルが存在しない(ENOENT)と特定できた\n");
    }

    /* 成功したシステムコールは errno を 0 にしない点に注意:
     * 「戻り値で成否を判定し、失敗のときだけ errno を見る」が鉄則。 */
    fd = open("/dev/null", O_RDONLY);
    printf("\nopen(/dev/null) = %d (成功。errno は見に行かない)\n", fd);
    close(fd);

    return 0;
}
