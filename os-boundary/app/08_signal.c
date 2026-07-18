/*
 * 08_signal.c  —  シグナル(1): sigaction で捕まえる
 * ------------------------------------------------------------
 * シグナル = カーネル/他プロセスからの「非同期通知」。
 *   SIGINT (Ctrl-C), SIGTERM (kill 既定), SIGHUP, ...
 * signal() は移植性・挙動に難があるため、実務では sigaction() を使う。
 *
 * 重要な制約: シグナルハンドラ内で呼べるのは「async-signal-safe」な
 * 関数だけ(write は OK、printf/malloc は NG)。ここでは
 * volatile sig_atomic_t のフラグを立てるだけにして、実処理は
 * メインループで行う定石を示す。
 *
 * 実行後、別ターミナルで  kill -TERM <PID>  または Ctrl-C で終了。
 * ------------------------------------------------------------
 */
#include "io_utils.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

/* ハンドラとメインの間で共有するフラグ。
 * volatile sig_atomic_t が唯一「ハンドラで安全に触れる」型。 */
static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_last_sig = 0;

static void on_signal(int signo)
{
    g_last_sig = signo;
    g_stop = 1;
    /* ここで printf は禁止(非同期シグナル安全でない)。
     * どうしても出したいなら write(2, ...) を使う。 */
    const char msg[] = "  [handler] シグナル受信、フラグを立てた\n";
    /* ベストエフォート出力。戻り値は無視するが、警告回避のため受ける。 */
    if (write(STDERR_FILENO, msg, sizeof(msg) - 1) < 0) { /* ignore */ }
}

int main(void)
{
    /* SA_RESTART: このシグナルで中断された遅い system call を自動再開。 */
    install_handler(SIGINT,  on_signal, SA_RESTART);
    install_handler(SIGTERM, on_signal, SA_RESTART);

    printf("PID=%d 実行中。Ctrl-C か  kill -TERM %d  で停止。\n",
           (int)getpid(), (int)getpid());

    /* メインループ: 実処理はここで行う(ハンドラは軽く済ませる)。 */
    while (!g_stop) {
        pause();   /* シグナルが来るまで眠る。来たら EINTR で起きる。 */
    }

    printf("シグナル %d を受けてクリーンに終了\n", (int)g_last_sig);
    return 0;
}
