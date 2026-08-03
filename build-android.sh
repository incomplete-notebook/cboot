#!/bin/bash
# build-android.sh - 构建 Aarch64-Android 静态二进制 (musl-libc)
#
# 背景: 之前用 aarch64-linux-gnu-gcc (glibc) -static 链接的二进制在 Android 上
#       运行报 "bad system call" (SIGSYS)。根因是 glibc 静态二进制在启动/posix_spawn
#       时使用 clone3 等 Android seccomp 过滤器拦截的系统调用，且 glibc 在 ELF note
#       中标记 "for GNU/Linux 3.7.0"，与 Android Bionic libc 不兼容。
#
# 修复: 改用 musl-libc 静态链接。musl 仅使用标准/保守的系统调用，静态二进制只依赖
#       内核 ABI (Android 内核提供)，无动态加载器、无 glibc 特有系统调用，
#       可在 Android 上直接运行。
#
# 依赖:
#   apt-get install gcc-aarch64-linux-gnu
#   # musl 源码 (aarch64 交叉编译):
#   apt-get source musl
#   cd musl-* && ./configure --target=aarch64 CROSS_COMPILE=aarch64-linux-gnu- \
#       --disable-shared --prefix=$PWD/../musl-aarch64 && make -j && make install
#
# 用法: ./build-android.sh [输出路径]

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

MUSL="${MUSL_PREFIX:-/tmp/musl-aarch64}"
GCCINC="$(aarch64-linux-gnu-gcc -print-file-name=include)"
CC=aarch64-linux-gnu-gcc
OUT="${1:-release/cboot-aarch64-android-v1.1.0}"

if [ ! -f "$MUSL/lib/libc.a" ]; then
    echo "错误: 未找到 musl aarch64 库 ($MUSL/lib/libc.a)" >&2
    echo "请先交叉编译 musl (见脚本头部说明)" >&2
    exit 1
fi

BUILD_DIR="$(mktemp -d)"
trap 'rm -rf "$BUILD_DIR"' EXIT

# 极限优化: -O3 + LTO + 段级 GC + 去除调试/异常表
CFLAGS="-nostdinc -isystem $MUSL/include -isystem $GCCINC -I. \
    -O3 -flto -ffunction-sections -fdata-sections \
    -fno-asynchronous-unwind-tables -fno-unwind-tables"

SRCS="commands/analyze/analyze.c commands/commands.c cupdate/cupdate.c \
cupdate/cupdate_lexer.c cupdate/cupdate_parser.c docgen/docgen.c \
domain/domain.c domain/core/core.c generator/generator.c main/main.c \
parser/parser.c typecheck/typecheck.c utils/utils.c"

echo "==> 编译源文件 (aarch64 + musl, 极限优化)"
OBJS=""
for s in $SRCS; do
    out="$BUILD_DIR/$(echo "$s" | tr '/' '_').o"
    $CC $CFLAGS -c "$s" -o "$out"
    OBJS="$OBJS $out"
done

echo "==> 静态链接 (musl libc, 无动态段/无 interpreter)"
mkdir -p "$(dirname "$OUT")"
$CC -static -no-pie -nostdlib -nostartfiles \
    -Wl,--gc-sections -Wl,--build-id=none -Wl,-z,max-page-size=0x10000 \
    "$MUSL/lib/crt1.o" "$MUSL/lib/crti.o" $OBJS "$MUSL/lib/crtn.o" \
    "$MUSL/lib/libc.a" -lgcc \
    -o "$OUT"

echo "==> 剥离符号"
aarch64-linux-gnu-strip "$OUT"

echo "==> 完成"
ls -la "$OUT"
file "$OUT"
