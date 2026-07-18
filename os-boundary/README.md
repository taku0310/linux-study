# OSとの境界(システムコール) 学習プロジェクト

組み込みLinuxエンジニア養成講座 第2回。プロセス/スレッド編に続き、
**ユーザ空間とカーネルの境界=システムコール**を実習で学ぶキットです。
POSIX/Linux API を使った動くサンプルと、体系的な教材で構成しています。

## 教材
- **[docs/os-boundary.md](docs/os-boundary.md)** — 本編。5テーマ + 理解度確認テスト10問。
  1. エラーと errno
  2. ファイルディスクリプタと I/O(everything is a file / lseek)
  3. ノンブロッキング I/O
  4. I/O多重化(select / poll / epoll)
  5. シグナル(sigaction / SIGCHLD 回収 / signalfd)
  + システムコールの仕組み(strace 観察)

## フォルダ構成
```
os-boundary/
├── app/        各テーマのサンプル (01〜11)
├── control/    システムコールの薄いラッパ (io_utils.c)
├── driver/     デバイスを FD 経由で読む層 (dev_read.c)
├── include/    ヘッダ (io_utils.h)
├── build/      ビルド成果物 (make で生成)
└── Makefile
```
前回の `project/`(プロセス/スレッド編)と同じ app/control/driver/include の
3層構成を踏襲しています。

## サンプル一覧
| ファイル | テーマ |
|----------|--------|
| `app/01_errno.c` | エラーと errno / 戻り値規約 |
| `app/02_fd_basic.c` | FD と open/read/write/close |
| `app/03_everything_is_file.c` | /proc を読む・lseek |
| `app/04_nonblock.c` | ノンブロッキング I/O と EAGAIN |
| `app/05_select.c` | I/O多重化: select |
| `app/06_poll.c` | I/O多重化: poll |
| `app/07_epoll.c` | I/O多重化: epoll(Linux固有) |
| `app/08_signal.c` | シグナル: sigaction 基本 |
| `app/09_sigchld.c` | シグナル: SIGCHLD でゾンビ回収 |
| `app/10_signalfd.c` | シグナル: signalfd + epoll に統合 |
| `app/11_syscall.c` | システムコールの仕組み(strace 観察) |

## ビルドと実行
```bash
make                      # 全サンプルを build/ にビルド
make run-02_fd_basic      # 指定サンプルを実行 (run-<名前>)
make strace-11_syscall    # strace 付きで syscall を観察
make clean                # build/ を掃除
```
`08_signal` / `10_signalfd` は常駐するので、別ターミナルから
`kill -TERM <PID>` か `Ctrl-C` で終了させて挙動を観察してください。

## 動作環境
- Ubuntu 24.04 / gcc 13 / glibc 2.39 / bash
- `strace` を使う場合: `sudo apt install strace`
- epoll・signalfd は Linux 固有 API(各サンプルで `_GNU_SOURCE` を定義)
