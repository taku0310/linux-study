# プロセスとスレッド 学習プロジェクト

組み込みLinuxエンジニア養成向けの、プロセス/スレッド実習キットです。
POSIX API を使った動くサンプルと、体系的な教材ドキュメントで構成しています。

## 教材
- **[docs/processes-and-threads.md](docs/processes-and-threads.md)** — 本編。
  ①プロセス〜⑭孤児プロセスまで14テーマ + 理解度確認テスト10問。

## フォルダ構成
```
project/
├── app/        各テーマのサンプルアプリ (01〜10, main_demo)
├── control/    システムコールの薄いラッパ (proc_utils.c)
├── driver/     デバイスを叩くドライバ層のプレースホルダ
├── include/    ヘッダ (proc_utils.h)
├── build/      ビルド成果物 (make で生成)
└── Makefile
```
`app`(業務ロジック) → `control`(OS機能の抽象化) → `driver`(デバイス)の
3層構成。実務でよくある「システムコールを直接散らさず control 層に集約」
という設計を小さく再現しています。

## サンプル一覧
| ファイル | テーマ |
|----------|--------|
| `app/01_process_basic.c` | ①プロセス / ⑫PID・TID |
| `app/02_fork.c` | ⑤fork() と COW によるメモリ独立 |
| `app/03_exec.c` | ⑥exec() (fork+exec の定石) |
| `app/04_wait.c` | ⑦wait() と終了ステータス判定 |
| `app/05_zombie.c` | ⑬ゾンビプロセス (Z/defunct) |
| `app/06_orphan.c` | ⑭孤児プロセスと再親付け |
| `app/07_pthread_basic.c` | ②スレッド / ⑧⑨pthread_create/join |
| `app/08_race_and_mutex.c` | ④共有メモリの罠とmutex |
| `app/09_pid_tid.c` | ⑫PID・TID をカーネル視点で厳密に |
| `app/10_context_switch.c` | ⑩コンテキストスイッチ / ⑪CFS |
| `app/main_demo.c` | 3層(app/control/driver)通し例 |

## ビルドと実行
```bash
make                     # 全サンプルを build/ にビルド
make run-02_fork         # 指定サンプルを実行 (run-<名前>)
make race                # データ競合デモ (mutexなし, -O0)
make mutex               # 正しい版 (mutexあり)
make clean               # build/ を掃除
```

## 動作環境
- Ubuntu 24.04 / gcc 13 / glibc 2.39 / bash
- `pidstat` を使う場合: `sudo apt install sysstat`
