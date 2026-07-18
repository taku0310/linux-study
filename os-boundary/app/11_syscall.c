/*
 * 11_syscall.c  —  システムコールの仕組み
 * ------------------------------------------------------------
 * 「libc の関数」と「システムコール」は別物。多くの libc 関数は
 * 内部でシステムコールを呼ぶ薄いラッパにすぎない。
 *
 * この例では同じ「1行の出力」を3通りで行う:
 *   (A) printf         : libc のバッファI/O。内部で write(2) を呼ぶ
 *   (B) write(1, ...)  : libc の write ラッパ(ほぼ直接 syscall)
 *   (C) syscall(SYS_write, ...) : ラッパを介さず番号指定で直接
 *
 * strace で観察すると (A)(B)(C) すべてが最終的に write システム
 * コールに落ちることが分かる。ユーザ空間→カーネル空間への遷移は
 * 特別な命令(x86-64 なら syscall 命令)で行われる。
 * ------------------------------------------------------------
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>   /* SYS_write, SYS_getpid */

int main(void)
{
    const char *b = "(B) libc の write ラッパ経由\n";
    const char *c = "(C) syscall() で直接 write\n";

    /* (A) libc のバッファ付き I/O。改行や fflush まで実際の write が
     *     遅延することもある。 */
    printf("(A) printf 経由(内部で write を呼ぶ)\n");
    fflush(stdout);   /* strace で順序を見やすくするため明示 flush */

    /* (B) libc の write() ラッパ。ほぼそのまま write syscall。 */
    if (write(STDOUT_FILENO, b, strlen(b)) < 0) { /* ignore */ }

    /* (C) ラッパを介さず、システムコール番号 SYS_write を直接指定。
     *     libc の write() が存在しない/使いたくない状況の最終手段。 */
    if (syscall(SYS_write, STDOUT_FILENO, c, strlen(c)) < 0) { /* ignore */ }

    /* getpid も同様。libc の getpid() と生 syscall は同じ結果。 */
    pid_t a = getpid();
    long  d = syscall(SYS_getpid);
    printf("\ngetpid()=%d  syscall(SYS_getpid)=%ld  (一致するはず)\n",
           (int)a, d);

    printf("\n観察: strace -e trace=write,getpid ./build/11_syscall\n");
    return 0;
}
