# ============================================================
# Makefile  —  「OSとの境界(システムコール)」学習プロジェクト
# ============================================================
# 使い方:
#   make                  全サンプルを build/ にビルド
#   make run-02_fd_basic  指定サンプルを実行 (run-<名前>)
#   make strace-11_syscall  strace 付きで syscall を観察
#   make clean            build/ を掃除
# ============================================================

CC      := gcc
# _GNU_SOURCE を使うサンプル(epoll/signalfd)があるが、各 .c 側で
# #define しているのでここでは付けない。-D_DEFAULT_SOURCE 相当は std=gnu11 で担保。
CFLAGS  := -std=gnu11 -Wall -Wextra -O2 -g -Iinclude
BUILD   := build
CONTROL := control/io_utils.c

APP_SRCS := $(wildcard app/*.c)
APP_BINS := $(patsubst app/%.c,$(BUILD)/%,$(APP_SRCS))

.PHONY: all clean

all: $(APP_BINS)

# 各サンプル = app/xxx.c + control 層(io_utils)
$(BUILD)/%: app/%.c $(CONTROL) | $(BUILD)
	$(CC) $(CFLAGS) $< $(CONTROL) -o $@

$(BUILD):
	mkdir -p $(BUILD)

# 実行ヘルパ:  make run-02_fd_basic のように使う
run-%: $(BUILD)/%
	@echo "----- run $< -----"
	@./$<

# strace 観察ヘルパ:  make strace-11_syscall
strace-%: $(BUILD)/%
	@command -v strace >/dev/null 2>&1 || { echo "strace 未インストール: sudo apt install strace"; exit 1; }
	strace -f -e trace=write,read,openat,epoll_wait,rt_sigaction ./$<

clean:
	rm -rf $(BUILD)/*
