/*
 * 09_sigchld.c  —  シグナル(2): SIGCHLD でゾンビ自動回収
 * ------------------------------------------------------------
 * 前回(プロセス/スレッド編)の伏線回収。
 * 子が終了するとカーネルは親に SIGCHLD を送る。これを契機に
 * waitpid(WNOHANG) をループで回して、終了した子を「取りこぼし
 * なく」回収するのが、常駐デーモンの定石。
 *
 * ポイント:
 *   - 複数の子がほぼ同時に死ぬと SIGCHLD は「まとめて1回」しか
 *     来ないことがある(標準シグナルはキューイングされない)。だから
 *     ハンドラ内で while ループ回収が必須。
 *   - ハンドラ内で使えるのは async-signal-safe な関数だけ。
 *     printf/snprintf は不可なので、自前の安全な整数出力を使う。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define N_CHILD 5

static volatile sig_atomic_t g_reaped = 0;

/* strlen も厳密には安全リストに無いので自前(単純ループは安全)。 */
static size_t strlen_safe(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* async-signal-safe な「文字列 + 10進整数 + 改行」出力。
 * write() のみを使うので、シグナルハンドラ内でも安全。 */
static void safe_report(const char *prefix, long value)
{
    char num[32];
    int i = (int)sizeof(num);
    num[--i] = '\n';
    if (value == 0) {
        num[--i] = '0';
    } else {
        long v = value;
        while (v > 0 && i > 0) {
            num[--i] = (char)('0' + v % 10);
            v /= 10;
        }
    }
    /* ベストエフォート出力。戻り値は無視するが、警告回避のため受ける。 */
    if (write(STDERR_FILENO, prefix, strlen_safe(prefix)) < 0) { /* ignore */ }
    if (write(STDERR_FILENO, num + i, sizeof(num) - (size_t)i) < 0) { /* ignore */ }
}

static void on_sigchld(int signo)
{
    (void)signo;
    int saved = errno;          /* ハンドラは errno を保存/復元するのが作法 */
    pid_t pid;
    /* 取りこぼし防止のループ回収。ここが肝。 */
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        g_reaped++;
        safe_report("  [SIGCHLD] 回収した子 PID=", (long)pid);
    }
    errno = saved;
}

int main(void)
{
    install_handler(SIGCHLD, on_sigchld, SA_RESTART | SA_NOCLDSTOP);

    printf("親 PID=%d が %d 個の子を生成\n", (int)getpid(), N_CHILD);
    fflush(stdout);
    for (int i = 0; i < N_CHILD; i++) {
        pid_t pid = fork();
        if (pid < 0) die("fork");
        if (pid == 0) {
            /* 子: バラバラの時間で終了させ、同時性を作る。 */
            usleep((useconds_t)(100000 * (i + 1)));
            _exit(0);
        }
    }

    /* 全部回収されるまで待つ。SIGCHLD で pause から起きる。 */
    while (g_reaped < N_CHILD)
        pause();

    printf("全 %d 子をゾンビ化させずに回収完了\n", g_reaped);
    return 0;
}
