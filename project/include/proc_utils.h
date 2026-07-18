/*
 * proc_utils.h
 * ------------------------------------------------------------
 * プロセス/スレッド制御ヘルパの宣言。
 *
 * 業務コードでは「システムコールの薄いラッパ」を control 層に
 * 集約し、app 層はそのラッパ経由で OS 機能を使う。こうすると
 *   - エラーハンドリングを一箇所に集約できる
 *   - ログ/計測を差し込みやすい
 *   - app 層のロジックが読みやすくなる
 * という利点がある。
 * ------------------------------------------------------------
 */
#ifndef PROC_UTILS_H
#define PROC_UTILS_H

#include <sys/types.h>   /* pid_t */
#include <pthread.h>

/* Linux 固有: gettid() を glibc 2.30+ で使うためのラッパ。
 * TID (カーネルから見たスレッドID = LWP) を返す。 */
pid_t get_tid(void);

/* 統一フォーマットで「誰が動いているか」を表示するヘルパ。
 * tag: 呼び出し元を示す任意の文字列。 */
void print_ids(const char *tag);

/* fork() のエラーチェック付きラッパ。失敗時は perror して exit。 */
pid_t xfork(void);

/* pthread_create() のエラーチェック付きラッパ。
 * pthread 系関数は errno を使わず戻り値でエラーを返す点に注意。 */
void xpthread_create(pthread_t *thread,
                     void *(*start_routine)(void *),
                     void *arg);

/* 子プロセスを 1 つ待って終了ステータスを人間可読な形で表示する。
 * 戻り値: 待った子の PID。 */
pid_t wait_and_report(void);

#endif /* PROC_UTILS_H */
