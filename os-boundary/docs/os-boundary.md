# OSとの境界(システムコール)— 組み込みLinuxエンジニア養成講座 第2回

対象: C言語 / gcc / Ubuntu 24.04 / bash
到達目標: ユーザ空間プログラムがカーネルの機能を「システムコール」で
呼び出す仕組みを理解し、ファイルディスクリプタ・I/O多重化・シグナルを
正しく扱えること。ここは**デバイスドライバ開発とROS2の直接の土台**。

> `os-boundary/` 配下の動くサンプルとセットです。
> ```bash
> cd os-boundary
> make          # build/ に全サンプル生成
> ```

---

## 0. 全体像 — なぜ「OSとの境界」なのか

前回はプロセス/スレッドという「実行の単位」を学びました。今回は、その
プロセスが**カーネルに仕事を頼む唯一の窓口=システムコール**を掘ります。

```
 ┌──────────────── ユーザ空間(あなたのCコード / libc)────────────────┐
 │  printf()   fopen()   pthread_create()  ...  ← libc(便利なラッパ)  │
 │       │        │                                                    │
 │   これらは内部で ↓ を呼ぶ                                            │
 │  write()  open()  read()  epoll_wait()  clone()  ← システムコール    │
 └───────────────────────────┬────────────────────────────────────────┘
              syscall 命令で特権レベルを切り替え(モード遷移)
 ┌───────────────────────────▼────────────────────────────────────────┐
 │  カーネル空間: ファイルシステム / ドライバ / スケジューラ / メモリ管理 │
 └─────────────────────────────────────────────────────────────────────┘
```

ドライバは「システムコール(open/read/write/ioctl)を受ける側」を書く仕事。
ROS2 の通信も epoll ベースのイベントループの上に乗ります。だから
**境界の作法(FD・errno・多重化・シグナル)を先に固める**のが本質的。

本モジュールで扱う5テーマ:
1. **エラーと errno** — システムコールの成否の伝わり方
2. **ファイルディスクリプタと I/O** — open/read/write/close/lseek と "everything is a file"
3. **ノンブロッキング I/O** — 待たない I/O、EAGAIN
4. **I/O多重化** — select / poll / epoll
5. **シグナル** — sigaction / SIGCHLD 回収 / signalfd

---

# ① エラーと errno

### なぜ必要か
システムコールは**必ず失敗しうる**(ファイルが無い、権限が無い、メモリ枯渇…)。
失敗を検出・分類できないと、堅牢な組み込みソフトは書けません。

### OS内部で何が起きているか
ほとんどのシステムコールは **「成功なら 0 以上、失敗なら -1 を返し、
グローバル(正確にはスレッドローカル)な `errno` に理由コードをセット」** する規約です。
`errno` は `<errno.h>` の `ENOENT`(無い)、`EACCES`(権限)、`EAGAIN`(今は無理)等の
マクロで判定します。

重要な性質:
- `errno` は**スレッドごとに独立**(TLS)。他スレッドの失敗に汚されない。
- **成功したシステムコールは errno を 0 に戻さない**。だから
  「戻り値で成否を判定し、失敗のときだけ errno を読む」。
- 後続の関数呼び出しが errno を上書きするので、**失敗直後に退避**する。

### 用語
| 用語 | 意味 |
|------|------|
| errno | 直近の失敗理由を表すスレッドローカルな整数 |
| perror(s) | `s: <errnoの説明>` を stderr に出す |
| strerror(e) | errno 値 → 説明文字列 |
| EINTR | シグナルで system call が中断された(＝リトライすべき) |
| EAGAIN | ノンブロッキングで「今はデータが無い」 |

### サンプル / 実行結果
`app/01_errno.c`:
```
open /no/such/file: No such file or directory
open() returned -1
errno         = 2
strerror(e)   = No such file or directory
→ ファイルが存在しない(ENOENT)と特定できた
open(/dev/null) = 3 (成功。errno は見に行かない)
```
`errno=2` が `ENOENT`。成功した2つ目の open は errno を見ていない点に注目。

### よくある間違い
- **戻り値を見ずに errno だけ見る** → 成功時の errno は不定なので誤判定。
- **失敗直後に別の関数を挟んでから errno を読む** → 上書きされている。
- `EINTR` を「エラー」として扱い中断 → 多くは**リトライすべき**(control 層の
  `read_all`/`write_all` が自動対応)。

---

# ② ファイルディスクリプタと I/O

### なぜ必要か
Linux では**ファイルも、デバイスも、パイプも、ソケットも、すべて
「ファイルディスクリプタ(FD)」という同じ抽象で操作**します。この
統一インタフェースを理解すれば、ドライバもネットワークも同じ道具で扱えます。

### OS内部で何が起きているか
FD は「プロセスが開いているファイルの表(file descriptor table)」への
**インデックス(小さな非負整数)**。`open()` はカーネル内に
`struct file` を用意し、その表の**最小の空き番号**を返します。
- `0` = 標準入力、`1` = 標準出力、`2` = 標準エラー(起動時に予約済み)
- だから最初の `open` は通常 `3` を返す。
- FD → `struct file` → `inode`/ドライバ、とカーネル内で辿られる。

`read(fd, buf, n)` / `write(fd, buf, n)` は「この FD に n バイト読み書き」を
カーネルに依頼するシステムコール。**戻り値は「実際に処理したバイト数」**で、
要求より少ないこと(短い read/write)があるのが重要。

### イメージ図
```
プロセスの file descriptor table
  fd 0 → stdin
  fd 1 → stdout
  fd 2 → stderr
  fd 3 → struct file → inode → ext4 上の /tmp/xxx.txt
  fd 4 → struct file → パイプバッファ
  fd 5 → struct file → ソケット
        (全部 read()/write() で同じように扱える)
```

### 用語
| API | 役割 |
|-----|------|
| `open(path,flags,mode)` | ファイルを開き FD を得る。O_RDONLY/O_WRONLY/O_CREAT/O_TRUNC/O_APPEND 等 |
| `read/write` | FD に対する読み書き。戻り値=実バイト数 |
| `close(fd)` | FD を解放(しないと FD リーク) |
| `lseek(fd,off,whence)` | 読み書き位置(オフセット)を移動。SEEK_SET/CUR/END |

### サンプル / 実行結果
`app/02_fd_basic.c`:
```
open("/tmp/os_boundary_fd_demo.txt") = fd 3 (0,1,2 は標準ストリームで予約済み)
read back 24 bytes: hello, file descriptor!
この行は write(1, ...) で直接出した
```
> 最後の「write(1,...)」行が先に見えることがあるのは、`printf` が
> **バッファリング**され、`write(1,...)` は即時だから。バッファの存在も
> 大事な学び(後述の syscall テーマに繋がる)。

`app/03_everything_is_file.c` — `/proc/self/status`(カーネルが生成する
仮想ファイル)をディスクのファイルと**全く同じ read() で**読む:
```
---- /proc/self/status (先頭 160 バイト) ----
Name:	03_everything_i
State:	R (running)
Pid:	3543
...
lseek(fd, 3, SEEK_SET) -> offset 3
read 3 bytes from offset 3: "345"
file size (lseek SEEK_END) = 10 bytes
```
`/proc` に実体は無いのに read できる = **"everything is a file"** の実演。
`lseek(fd,0,SEEK_END)` はファイルサイズ取得の常套句。

### Linuxコマンドで確認
```bash
ls -l /proc/<PID>/fd/      # プロセスが開いている FD 一覧(シンボリックリンク)
lsof -p <PID>              # 同上をより詳しく
cat /proc/self/status      # サンプルが読んだのと同じ仮想ファイル
```

### よくある間違い
- **close 忘れ** → FD リーク。`ulimit -n`(既定1024)到達で `EMFILE`。
- **read/write の戻り値を「全部処理された」と決めつける** → 短い read/write を
  取りこぼす。`read_all`/`write_all` のようにループで埋める。
- `O_CREAT` 時に第3引数(mode)を渡し忘れる → パーミッションが不定に。

---

# ③ ノンブロッキング I/O

### なぜ必要か
既定の `read` は「データが来るまで**待つ(ブロック)**」。1つの FD に
張り付くと他の仕事ができません。イベント駆動(次テーマの多重化)の前提として、
「データが無ければ即戻る」I/O が必要です。

### OS内部で何が起きているか
FD に `O_NONBLOCK` フラグを立てると、`read`/`write` は完了できないとき
**ブロックせず即座に `-1` / `errno=EAGAIN`(=EWOULDBLOCK)** を返します。
「今は無理、あとで来て」という意思表示です。`fcntl(fd, F_SETFL, ...)` で設定します。

### サンプル / 実行結果
`app/04_nonblock.c`(まだ何も書いていないパイプを読む):
```
data無し: read() が即 -1 / errno=EAGAIN で戻った(待たない)
書き込み後の read(): 17 バイト "now there is data"
```
書く前は EAGAIN で即戻り、書いた後は読める。**ブロックしない**のがポイント。

### よくある間違い
- `EAGAIN` を致命エラー扱いして落ちる → ノンブロッキングでは**正常な合図**。
- ノンブロッキング + ビジーループ(`while(read()<0)`)で **CPU 100%** →
  必ず epoll 等で「読めるようになるまで寝る」を組み合わせる。

---

# ④ I/O多重化(select / poll / epoll)★本テーマの山場

### なぜ必要か
センサ・通信・タイマなど**複数の入力を1スレッドで同時に待ちたい**。
スレッドを増やす手もあるが、数千接続では非効率。「1スレッドで多数の FD を
待つ」のが I/O多重化で、ネットワークサーバ・ROS2 executor の心臓部です。

### OS内部で何が起きているか
3つとも「複数 FD を監視し、**どれかが読み書き可能になったら起こす**」
システムコール。違いはスケーラビリティ:

| | select | poll | epoll(Linux固有) |
|---|--------|------|------------------|
| 監視FD数の上限 | FD_SETSIZE(通常1024) | 無制限 | 無制限 |
| 監視集合の受け渡し | 毎回 fd_set を作り直し | pollfd配列 | カーネルに1度登録 |
| 計算量(1回) | O(N) 全走査 | O(N) 全走査 | **O(発生数)** に近い |
| 大量FDでの性能 | 悪い | 悪い | 良い |
| 使いどころ | 少数FD・移植性重視 | 中程度 | 高性能・多数FD |

**epoll の肝**: 監視対象を `epoll_ctl(ADD)` でカーネル側に**据え置き**、
`epoll_wait` は「実際にイベントが起きた FD **だけ**」を返す。だから
FD が1万個あっても、暇な FD を毎回走査しない。

### イメージ図(epoll)
```
epoll インスタンス(それ自体も FD)
  ├─ 登録: pipe0(EPOLLIN)
  ├─ 登録: pipe1(EPOLLIN)
  └─ 登録: socket(EPOLLIN)   ← ここまで epoll_ctl(ADD) で1度だけ

  epoll_wait() ──▶ 「今読めるのは pipe0 だけ」と、起きたものだけ返す
```

### サンプル / 実行結果
`05_select.c` / `06_poll.c` / `07_epoll.c` は**同じ動作**(2パイプを子が
1秒・2秒後に書き、親が多重化で待つ)。epoll版:
```
epoll_wait(): 1 個のイベント発生
  fd 3 から受信: "pipe0: 1秒後に到着"
epoll_wait(): 1 個のイベント発生
  fd 3: EOF、epoll から削除
epoll_wait(): 1 個のイベント発生
  fd 5 から受信: "pipe1: 2秒後に到着"
両パイプ完了。epoll ループ終了。
```
3実装で**出力が同じ**なのが狙い。APIの違い=同じ問題への進化と捉える。

### 用語
| 用語 | 意味 |
|------|------|
| level-triggered (LT) | 「読める状態が続く限り」通知(既定、初心者向け) |
| edge-triggered (ET) | 「状態が変化した瞬間だけ」通知(EPOLLET、高速だが要ドレイン) |
| EPOLLIN/EPOLLOUT | 読み取り可能/書き込み可能の監視 |
| EPOLLHUP | 相手が切断した |

### よくある間違い
- **select で FD_SET を毎回作り直さない** → select は集合を書き換えるので
  ループごとに再構築が必須。
- epoll の **ET モードで読み切らない** → 次の通知が来ず「固まる」。ET では
  `EAGAIN` が出るまで read し切る。
- 監視をやめた FD を `EPOLL_CTL_DEL`/`close` し忘れ → リーク・誤通知。

### 実務での使われ方
nginx・Redis・libuv(Node.js)・ROS2 の rmw 層など、高性能I/Oは基本 epoll。
「1スレッド + epoll でイベントを捌く」= **リアクタパターン**の実装そのもの。

---

# ⑤ シグナル(sigaction / SIGCHLD / signalfd)

### なぜ必要か
`Ctrl-C` での停止、`kill` での終了要求、子プロセスの死(SIGCHLD)、
タイマ満了など、**非同期の通知**をカーネルはシグナルで届けます。
これを正しく捌けないと、クリーンシャットダウンもゾンビ回収もできません。

### OS内部で何が起きているか
シグナルは「プロセスへの非同期割り込み」。配送されると、実行中のコードが
中断され**シグナルハンドラ**が呼ばれます(既定動作は終了・無視など)。
歴史的な `signal()` は挙動が移植性に欠けるため、実務では **`sigaction()`** を使います。

**厳しい制約 — async-signal-safety**:
ハンドラは「いつでも」割り込むので、`printf`/`malloc` 等**再入不可の関数は
呼べない**。安全に呼べるのは `write`/`_exit` など限られた関数(man 7 signal-safety)。
定石は **「ハンドラは `volatile sig_atomic_t` のフラグを立てるだけ、実処理は
メインループ」**。

### 用語
| 用語 | 意味 |
|------|------|
| sigaction | シグナルハンドラを登録する推奨API |
| volatile sig_atomic_t | ハンドラとメインで安全に共有できる唯一の型 |
| SA_RESTART | このシグナルで中断された遅い syscall を自動再開 |
| async-signal-safe | ハンドラ内で呼んでも安全な関数の分類 |
| SIGKILL/SIGSTOP | 捕捉・無視できない特別なシグナル |

### サンプル(1): sigaction 基本
`app/08_signal.c`(`kill -TERM` で停止):
```
  [handler] シグナル受信、フラグを立てた
PID=4270 実行中。...
シグナル 15 を受けてクリーンに終了
```
ハンドラはフラグを立てるだけ、終了処理はメインループ。SIGTERM=15。

### サンプル(2): SIGCHLD でゾンビ回収【前回の伏線回収】
`app/09_sigchld.c` — 子が終わると SIGCHLD が来る。それを機に
`waitpid(-1,...,WNOHANG)` を**ループ回収**:
```
親 PID=4263 が 5 個の子を生成
  [SIGCHLD] 回収した子 PID=4264
  ...
  [SIGCHLD] 回収した子 PID=4268
全 5 子をゾンビ化させずに回収完了
```
> **なぜ while ループか**: 標準シグナルは**キューイングされない**。5子が
> ほぼ同時に死ぬと SIGCHLD が1回しか来ないことがある。1回の通知で
> `while(waitpid(...WNOHANG)>0)` を回し、**溜まった子を全部回収**する。

### サンプル(3): signalfd で「シグナルもFDに」【集大成】
`app/10_signalfd.c` — シグナルを**FDから read できる普通のイベント**にし、
epoll ループに一元化する:
```
PID=4272 イベントループ実行中。...
signalfd 経由でシグナル 2 を受信 → 終了
クリーンに終了しました
```
手順: ①対象シグナルを `sigprocmask` でブロック → ②`signalfd()` でFD化 →
③epoll で待つ。これで**ソケット・パイプ・タイマ・シグナルを1つの epoll で
待てる**。ハンドラでない=`printf` も安全に使える。実務のイベント駆動設計の完成形。

### よくある間違い
- ハンドラ内で `printf`/`malloc` を呼ぶ → **未定義動作**。フラグ+メインループへ。
- SIGCHLD ハンドラで `wait` を**1回だけ** → 取りこぼしでゾンビ残留。
- ハンドラで errno を保存・復元しない → メイン側の errno を壊す(09で対応済)。
- `SIGKILL`/`SIGSTOP` を捕まえようとする → 不可能。

---

# システムコールの仕組み(全テーマの背骨)

### 何が起きているか
libc の関数の多くは**システムコールの薄いラッパ**。`printf` も最終的には
`write` システムコールに落ちます。ユーザ空間→カーネル空間の移行は、
CPU の特別命令(x86-64 は `syscall` 命令)で**特権レベルを切り替え**て行われ、
カーネルは**システムコール番号**で処理を振り分けます。

### サンプル / strace 観察
`app/11_syscall.c` は同じ出力を (A)printf / (B)write / (C)syscall(SYS_write) の
3通りで行います。`strace` で観察すると:
```
$ strace -e trace=write,getpid ./build/11_syscall
write(1, "(A) printf ...", 45) = 45
write(1, "(B) libc ...",   35) = 35
write(1, "(C) syscall() ...", 30) = 30
getpid()                         = 4635
getpid()                         = 4635      ← libc getpid と生 syscall の2回
```
**(A)(B)(C) すべてが `write` システムコールに落ちる**ことが目で見える。
これが「libcは便利ラッパ、本体はsyscall」という事実の証明。

```bash
make strace-11_syscall     # 用意した観察ターゲット
strace -c ./build/02_fd_basic   # syscall ごとの回数・時間サマリ
```

### 実務での意味
- `strace` は「どのシステムコールで、どの errno で失敗したか」を暴く
  **最強のデバッグ道具**。ファイルが開けない・権限エラー・ハングの原因追跡に必須。
- ドライバ開発とは、この `open/read/write/ioctl` を**カーネル側で受ける**
  `file_operations` を書くこと。だから境界の理解がそのまま効く。

---

# Linuxコマンドで確認(まとめ)

| コマンド | 用途 |
|----------|------|
| `strace -f -e trace=...` | システムコールを追跡(最重要) |
| `strace -c` | syscall 回数・時間のサマリ |
| `ltrace` | ライブラリ関数呼び出しを追跡 |
| `ls -l /proc/<PID>/fd` | プロセスの開いている FD 一覧 |
| `lsof -p <PID>` | 開いているファイル/ソケット詳細 |
| `cat /proc/<PID>/status` | プロセス状態(仮想ファイル) |
| `ulimit -n` | FD 数の上限確認 |

---

# よくある間違い(総まとめ)

1. 戻り値を見ずに errno を読む(成功時 errno は不定)。
2. read/write の**短い転送**を取りこぼす → ループで埋める。
3. `EINTR` を error 扱い → リトライすべき。
4. `EAGAIN` を致命扱い → ノンブロッキングの正常な合図。
5. close 忘れの FD リーク → `EMFILE`。
6. select の fd_set を毎回作り直さない。
7. epoll ET で読み切らない → 通知が止まる。
8. シグナルハンドラで `printf`/`malloc`(async-signal-unsafe)。
9. SIGCHLD ハンドラで `wait` を1回だけ → ゾンビ残留。
10. ハンドラで errno を保存/復元しない。

---

# 演習問題

1. `02_fd_basic.c` を `O_APPEND` で開き直し、2回実行すると内容が
   追記されることを確認せよ。`O_TRUNC` との違いを説明せよ。
2. `read_all`/`write_all` を使って「ファイルコピー `cp` 相当」を実装せよ
   (`mycp src dst`)。短い read/write を正しく扱うこと。
3. `04_nonblock.c` を土台に、標準入力(fd 0)をノンブロッキングにして
   「入力が無ければ '.' を出し続け、来たら表示」するループを書け。
4. `07_epoll.c` に**標準入力(fd 0)**も監視対象として追加し、
   キーボード入力とパイプを同じ epoll で捌け。
5. `09_sigchld.c` を改造し、`waitpid` を**1回だけ**にすると
   高速に子を量産したときにゾンビが残ることを `ps` で観察せよ。

# 発展課題

1. **ミニ echo サーバ**: UNIXドメインソケット + epoll で複数クライアントを
   1スレッドで捌け。`10_signalfd.c` の signalfd も epoll に足し、
   SIGTERM でクリーンシャットダウンせよ。
2. **timerfd**: `timerfd_create` で周期タイマを FD 化し、epoll で
   「1秒ごとの処理 + ソケット受信」を1ループに統合せよ(制御周期の作り方)。
3. **edge-triggered epoll**: 課題1を `EPOLLET` に変え、`EAGAIN` まで
   読み切る実装に直し、LT との違いを体感せよ。
4. **strace で謎解き**: わざと権限エラー(`open("/etc/shadow",O_RDONLY)` を
   一般ユーザで)を起こし、`strace` で `EACCES` を特定する練習をせよ。
5. **自作 write ラッパ**: `syscall(SYS_write,...)` だけで `puts` 相当を実装し、
   libc を使わずに文字列出力せよ(ドライバ側の気持ちを掴む)。

---

# 理解度確認テスト(全10問)

**Q1.** システムコールの一般的なエラー通知規約として正しいものは。
- (a) 失敗時に例外を投げる
- (b) 失敗時に -1 を返し errno に理由をセットする
- (c) 失敗時に必ず 0 を返す
- (d) errno を 0 にして成功を示す

**Q2.** ファイルディスクリプタ 0, 1, 2 が通常割り当てられているものは。
- (a) 最初に open した3ファイル
- (b) 標準入力・標準出力・標準エラー
- (c) カーネルログ・dmesg・syslog
- (d) stdin だけ(1,2 は未使用)

**Q3.** "everything is a file" を最もよく表すのはどれか。
- (a) すべてのデータは1つの巨大ファイルに保存される
- (b) 通常ファイル・デバイス・パイプ・ソケットを同じ read/write で扱える
- (c) すべてのファイルが /proc に置かれる
- (d) ファイル名は必ず拡張子を持つ

**Q4.** `O_NONBLOCK` を立てた FD で、読めるデータが無いとき `read` は。
- (a) データが来るまでブロックする
- (b) 0 を返す(EOF)
- (c) -1 を返し errno=EAGAIN
- (d) プロセスを終了させる

**Q5.** select / poll / epoll のうち、監視対象をカーネルに一度登録しておき
大量FDでも高効率なのはどれか。
- (a) select
- (b) poll
- (c) epoll
- (d) 3つとも同じ計算量

**Q6.** `read`/`write` の戻り値について正しいものは。
- (a) 常に要求したバイト数を返す
- (b) 要求より少ないバイト数を返すことがある(短い転送)
- (c) 成功時は必ず 0
- (d) バイト数ではなく FD を返す

**Q7.** システムコールが `EINTR` を返したとき、多くの場合の正しい対応は。
- (a) プログラムを異常終了させる
- (b) その system call をやり直す(リトライ)
- (c) errno を無視して先へ進む
- (d) FD を close する

**Q8.** シグナルハンドラ内で呼んでも安全(async-signal-safe)なのは。
- (a) printf
- (b) malloc
- (c) write
- (d) fopen

**Q9.** 複数の子がほぼ同時に終了するとき、SIGCHLD ハンドラで
ゾンビを取りこぼさないための正しい書き方は。
- (a) waitpid を1回だけ呼ぶ
- (b) `while (waitpid(-1,&s,WNOHANG) > 0) { ... }` でループ回収する
- (c) sleep してから wait する
- (d) ハンドラ内で fork し直す

**Q10.** `printf("hi\n")` を strace で観察すると、最終的に呼ばれる
システムコールは(概ね)どれか。
- (a) open
- (b) read
- (c) write
- (d) printf という名の system call

---

## 解答・解説

**A1: (b)** — 「-1 を返し errno に理由」が基本規約。成功時の errno は不定なので
(d) は誤り。C のシステムコール層に例外は無い((a)不可)。

**A2: (b)** — 0=stdin, 1=stdout, 2=stderr が起動時に予約。だから最初の
open は通常 3 を返す。

**A3: (b)** — デバイスも /proc もソケットも同じ FD + read/write で扱える、が
Unix の統一思想。(a)(c)(d) は誤解。

**A4: (c)** — ノンブロッキングでは「今は無理」を -1/EAGAIN で即返す。
ブロッキング((a))とは対照的。0(EOF)は相手が閉じたとき。

**A5: (c)** — epoll は監視集合をカーネルに据え置き、起きたイベントだけ返す。
select/poll は毎回全FDを渡し O(N) 走査。

**A6: (b)** — 短い read/write があり得る(パイプ/ソケット/シグナル割り込み)。
だから `read_all`/`write_all` のようにループで埋める。

**A7: (b)** — EINTR は「シグナルで中断」。多くはリトライすべき
(SA_RESTART か手動ループ)。

**A8: (c)** — write は async-signal-safe。printf/malloc/fopen は不可
(再入・ロックの危険)。ハンドラはフラグを立てるだけが定石。

**A9: (b)** — 標準シグナルはキューイングされないので、1回の SIGCHLD で
`while(waitpid(...WNOHANG)>0)` を回して溜まった子を全回収する。

**A10: (c)** — printf は libc のバッファ I/O だが、最終的に `write`
システムコールで出力される。「libcはラッパ、本体はsyscall」。

---

### 採点の目安
- 9〜10問: 境界の基礎は盤石。次は IPC(ソケット/共有メモリ)や
  ファイルI/Oの深掘り、そしてカーネルモジュールへ。
- 6〜8問: 概念はOK。errno規約・多重化3種の違い・シグナル安全性を再確認。
- 5問以下: 第①②④⑤テーマのサンプルを実際に動かし、`strace` で
  syscall を観察しながら再読を推奨。
