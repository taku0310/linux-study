# ============================================================
# Makefile  —  プロセスとスレッド 学習用プロジェクト
# ============================================================
# 使い方:
#   make            全サンプルを build/ にビルド
#   make run-02_fork  02_fork を実行
#   make race       mutex なし(データ競合)版を実行
#   make mutex      mutex あり(正しい)版を実行
#   make clean      build/ を掃除
# ============================================================

CC      := gcc
# -Wall -Wextra : 警告を最大限有効化(業務では -Werror も推奨)
# -g            : デバッグ情報(gdb 用)
# -pthread      : POSIX スレッド(コンパイル/リンク両方に必要)
CFLAGS  := -std=c11 -Wall -Wextra -O2 -g -pthread -Iinclude
LDFLAGS := -pthread

BUILD   := build
CONTROL := control/proc_utils.c
DRIVER  := driver/driver_stub.c

# app/ 配下の *.c を自動収集(main_demo は driver も要るので別扱い)
APP_SRCS := $(filter-out app/main_demo.c,$(wildcard app/*.c))
APP_BINS := $(patsubst app/%.c,$(BUILD)/%,$(APP_SRCS))

.PHONY: all clean race mutex

all: $(APP_BINS) $(BUILD)/main_demo

# 各サンプル = app/xxx.c + control 層
$(BUILD)/%: app/%.c $(CONTROL) | $(BUILD)
	$(CC) $(CFLAGS) $< $(CONTROL) -o $@ $(LDFLAGS)

# main_demo は driver 層もリンクする
$(BUILD)/main_demo: app/main_demo.c $(CONTROL) $(DRIVER) | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# build ディレクトリを用意(order-only 前提)
$(BUILD):
	mkdir -p $(BUILD)

# 実行ヘルパ:  make run-02_fork のように使う
run-%: $(BUILD)/%
	@echo "----- run $< -----"
	@./$<

# 08 のデータ競合デモは同一ソースをマクロ違いで2種ビルド。
# race は -O0 でビルドする点に注意: -O2 だとコンパイラが
# ループを畳み込み(g_counter += N)、競合が観測しづらくなるため、
# 「非アトミックな read-modify-write が並行する」様子を見せる目的で
# 最適化を切る。
race: | $(BUILD)
	$(CC) -std=c11 -Wall -Wextra -O0 -g -pthread -Iinclude app/08_race_and_mutex.c $(CONTROL) -o $(BUILD)/08_race $(LDFLAGS)
	@./$(BUILD)/08_race

mutex: | $(BUILD)
	$(CC) $(CFLAGS) -DUSE_MUTEX app/08_race_and_mutex.c $(CONTROL) -o $(BUILD)/08_mutex $(LDFLAGS)
	@./$(BUILD)/08_mutex

clean:
	rm -rf $(BUILD)/*
