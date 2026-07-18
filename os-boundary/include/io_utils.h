/*
 * io_utils.h
 * ------------------------------------------------------------
 * 「OSとの境界(システムコール)」を扱う薄いラッパ群の宣言。
 *
 * システムコールは戻り値と errno でエラーを伝える。この規約を
 * 毎回書くと app 層が汚れるので、チェック付きラッパを control 層に
 * 集約する。前回(プロセス/スレッド)の proc_utils と同じ思想。
 * ------------------------------------------------------------
 */
#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <sys/types.h>   /* ssize_t */
#include <stddef.h>      /* size_t  */
#include <signal.h>      /* sighandler / sigaction */

/* エラーメッセージを errno 付きで出して即終了する(perror + exit)。 */
void die(const char *msg);

/* open() のチェック付きラッパ。失敗時は die。 */
int xopen(const char *path, int flags, mode_t mode);

/* read() を「n バイト読み切る」まで繰り返す。
 * EINTR(シグナル割り込み)は自動リトライ。
 * 戻り値: 実際に読めたバイト数(EOF なら n 未満)。負なら error。 */
ssize_t read_all(int fd, void *buf, size_t n);

/* write() を「n バイト書き切る」まで繰り返す。EINTR は自動リトライ。
 * 部分書き込み(短い write)も繰り返して埋める。
 * 戻り値: 書けたバイト数(= n)か、負なら error。 */
ssize_t write_all(int fd, const void *buf, size_t n);

/* fd を非ブロッキング(O_NONBLOCK)に設定する。失敗時 die。 */
void set_nonblock(int fd);

/* sigaction() の薄いラッパ。handler を signo に登録する。
 * flags には SA_RESTART / SA_SIGINFO などを渡せる(0 でも可)。
 * 失敗時 die。 */
void install_handler(int signo, void (*handler)(int), int flags);

#endif /* IO_UTILS_H */
