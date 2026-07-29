#!/bin/bash
# test_update.sh - cboot update 和 adjust 功能综合测试脚本
#
# 测试内容:
#   1. 创建 cboot 项目并生成代码
#   2. 修改生成的源码（新增函数、修改宏值、修改结构体、删除函数、修改返回类型注释）
#   3. 执行 cboot update，检查 .cboot 是否正确同步了所有修改
#   4. 删除代码后重新 gen，验证生成的代码包含正确的定义
#   5. 测试 adjust 命令（通过批处理脚本模拟交互操作）
#
# 使用: bash test_update.sh

# ====================================================================
# 配置
# ====================================================================

CBOOT="/workspace/cboot-github/build/main/main"
TEST_DIR="/workspace/cboot-github/test_update"
WORK_DIR=$(mktemp -d)
PROJ_DIR="$WORK_DIR/calc_project"

# 测试计数
PASS=0
FAIL=0

# ====================================================================
# 辅助函数
# ====================================================================

title() {
    echo ""
    echo "================================================================"
    echo "  $1"
    echo "================================================================"
}

assert_exists() {
    if [ -f "$1" ]; then
        echo "  [PASS] 文件存在: $1"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] 文件不存在: $1"
        FAIL=$((FAIL + 1))
    fi
}

assert_dir_exists() {
    if [ -d "$1" ]; then
        echo "  [PASS] 目录存在: $1"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] 目录不存在: $1"
        FAIL=$((FAIL + 1))
    fi
}

assert_contains() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (在 $file 中未找到 '$pattern')"
        FAIL=$((FAIL + 1))
    fi
}

assert_not_contains() {
    local file="$1"
    local pattern="$2"
    local desc="$3"
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "  [FAIL] $desc (在 $file 中不应存在 '$pattern')"
        FAIL=$((FAIL + 1))
    else
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
    fi
}

# 运行 cboot 命令（输出重定向到日志，避免 SIGPIPE）
run_cboot() {
    "$CBOOT" "$@" >"$WORK_DIR/cboot_output.log" 2>&1
    return $?
}

# ====================================================================
# 准备工作
# ====================================================================

title "准备工作"

chmod +x "$CBOOT" 2>/dev/null
chmod +x "$TEST_DIR/test_update.sh" 2>/dev/null

echo "  cboot 可执行文件: $CBOOT"
echo "  测试脚本目录: $TEST_DIR"
echo "  工作目录: $WORK_DIR"

if [ ! -x "$CBOOT" ]; then
    echo "  [错误] cboot 可执行文件不存在或不可执行: $CBOOT"
    exit 1
fi
echo "  [OK] cboot 可执行文件就绪"

# ====================================================================
# 测试 1: 创建项目并生成代码
# ====================================================================

title "测试 1: 创建 cboot 项目并生成代码"

# cboot 批处理模式在当前目录生成文件，需要先创建项目目录
mkdir -p "$PROJ_DIR"
cd "$PROJ_DIR"

run_cboot "$TEST_DIR/create.cboot"
echo "  cboot 退出码: $?"

# 验证项目结构
assert_exists "$PROJ_DIR/.cboot"
assert_exists "$PROJ_DIR/CMakeLists.txt"
assert_dir_exists "$PROJ_DIR/calc"
assert_exists "$PROJ_DIR/calc/.cboot"
assert_exists "$PROJ_DIR/calc/calc.c"
assert_exists "$PROJ_DIR/calc/calc.h"

# ====================================================================
# 测试 2: 验证初始生成的代码
# ====================================================================

title "测试 2: 验证初始生成的代码"

CALC_C="$PROJ_DIR/calc/calc.c"
CALC_H="$PROJ_DIR/calc/calc.h"
CALC_CBOOT="$PROJ_DIR/calc/.cboot"
PROJ_CBOOT="$PROJ_DIR/.cboot"

echo "  --- calc.c 内容验证 ---"
assert_contains "$CALC_C" "calc_add" "calc.c 包含 add 函数"
assert_contains "$CALC_C" "calc_sub" "calc.c 包含 sub 函数"
assert_contains "$CALC_C" "calc_mul" "calc.c 包含 mul 函数"
assert_contains "$CALC_C" "MAX_VALUE 100" "calc.c 宏 MAX_VALUE 值为 100"
assert_contains "$CALC_C" "int x;" "calc.c 结构体 Point 包含成员 x"
assert_contains "$CALC_C" "int y;" "calc.c 结构体 Point 包含成员 y"

echo "  --- calc.h 内容验证 ---"
assert_contains "$CALC_H" "calc_add" "calc.h 声明 add 函数"
assert_contains "$CALC_H" "calc_sub" "calc.h 声明 sub 函数"
assert_contains "$CALC_H" "calc_mul" "calc.h 声明 mul 函数"

echo "  --- calc/.cboot 内容验证 ---"
assert_contains "$CALC_CBOOT" "void add int" ".cboot 定义 add 函数 (返回 int)"
assert_contains "$CALC_CBOOT" "void sub int" ".cboot 定义 sub 函数"
assert_contains "$CALC_CBOOT" "void mul int" ".cboot 定义 mul 函数"
assert_contains "$CALC_CBOOT" "def MAX_VALUE" ".cboot 定义 MAX_VALUE 宏"
assert_contains "$CALC_CBOOT" "struct Point" ".cboot 定义 Point 结构体"

# ====================================================================
# 测试 3: 修改生成的源代码
# ====================================================================

title "测试 3: 修改生成的源代码 calc.c"

cd "$PROJ_DIR"

echo "  执行 5 项修改:"

# 修改 1: 新增函数 div（追加到文件末尾）
echo "  [1] 新增函数 div"
printf '\nint calc_div(int a, int b) {\n    return a / b;\n}\n' >> "$CALC_C"

# 修改 2: 修改宏 MAX_VALUE 为 200
echo "  [2] 修改宏 MAX_VALUE 为 200"
sed -i 's/#define MAX_VALUE 100/#define MAX_VALUE 200/' "$CALC_C"

# 修改 3: 结构体 Point 新增成员 z（在 int y; 之后）
echo "  [3] 结构体 Point 新增成员 z"
sed -i '/int y;/a\    int z;' "$CALC_C"

# 修改 4: 删除函数 mul（从 "int calc_mul" 到下一个 "}" 行）
echo "  [4] 删除函数 mul"
sed -i '/^int calc_mul/,/^}/d' "$CALC_C"

# 修改 5: 修改函数 add 的返回类型注释（int 改为 double，并添加注释）
# 注意: 不使用 long，因为 cboot 的 typecheck 将 long 视为限定符会被剥离
echo "  [5] 修改函数 add 的返回类型注释"
sed -i '/^int calc_add/i\// 两数相加 - 返回类型注释已修改' "$CALC_C"
sed -i 's/^int calc_add(int a, int b)/double calc_add(int a, int b)/' "$CALC_C"

echo ""
echo "  --- 修改后的 calc.c 关键内容 ---"
echo "  -----------------------------------"
grep -n "MAX_VALUE\|int z;\|calc_div\|calc_mul\|calc_add\|返回类型" "$CALC_C" 2>/dev/null || echo "  (无匹配)"
echo "  -----------------------------------"

# 验证修改已生效
assert_contains "$CALC_C" "calc_div" "calc.c 已新增 div 函数"
assert_contains "$CALC_C" "MAX_VALUE 200" "calc.c 宏 MAX_VALUE 已改为 200"
assert_contains "$CALC_C" "int z;" "calc.c 结构体 Point 已新增成员 z"
assert_not_contains "$CALC_C" "calc_mul" "calc.c 已删除 mul 函数"
assert_contains "$CALC_C" "double calc_add" "calc.c add 函数返回类型已改为 double"
assert_contains "$CALC_C" "返回类型注释已修改" "calc.c add 函数注释已修改"

# ====================================================================
# 测试 4: 执行 cboot update
# ====================================================================

title "测试 4: 执行 cboot update 同步 .cboot"

# 将 modify.cboot 复制到项目目录（calc/.cboot 引用是相对路径）
cp "$TEST_DIR/modify.cboot" "$PROJ_DIR/"
cd "$PROJ_DIR"

echo "  执行: cboot modify.cboot"
run_cboot modify.cboot
echo "  cboot 退出码: $?"

# ====================================================================
# 测试 5: 验证 .cboot 正确同步了所有修改
# ====================================================================

title "测试 5: 验证 .cboot 正确同步了所有修改"

echo "  --- calc/.cboot 同步验证 ---"

# 验证 1: 新增函数 div 已同步到 .cboot
assert_contains "$CALC_CBOOT" "void div int" ".cboot 已同步新增 div 函数"

# 验证 2: 宏 MAX_VALUE 值已更新为 200
assert_contains "$CALC_CBOOT" 'value "200"' ".cboot 已同步 MAX_VALUE 值为 200"

# 验证 3: 结构体 Point 新增成员 z 已同步
assert_contains "$CALC_CBOOT" "mem z int" ".cboot 已同步 Point 新增成员 z"

# 验证 4: 函数 mul 已从 .cboot 删除
assert_not_contains "$CALC_CBOOT" "void mul int" ".cboot 已删除 mul 函数定义"

# 验证 5: 函数 add 返回类型已同步为 double
assert_contains "$CALC_CBOOT" "void add double" ".cboot 已同步 add 返回类型为 double"

# 验证 6: code 字段包含修改后的代码内容
assert_contains "$CALC_CBOOT" "calc_div" ".cboot code 字段包含 div 函数"
assert_contains "$CALC_CBOOT" "MAX_VALUE 200" ".cboot code 字段包含修改后的宏值"
assert_contains "$CALC_CBOOT" "返回类型注释已修改" ".cboot code 字段包含修改后的注释"

echo ""
echo "  --- calc.h 同步验证 ---"
assert_contains "$CALC_H" "calc_div" "calc.h 已声明 div 函数"
assert_not_contains "$CALC_H" "calc_mul" "calc.h 已移除 mul 函数声明"
assert_contains "$CALC_H" "double calc_add" "calc.h add 声明返回类型已改为 double"

# ====================================================================
# 测试 6: 删除代码后重新 gen
# ====================================================================

title "测试 6: 删除代码后重新 gen，验证生成的代码包含正确的定义"

cd "$PROJ_DIR"

echo "  删除生成的 calc.c 和 calc.h ..."
rm -f calc/calc.c calc/calc.h
echo "  [INFO] calc.c 和 calc.h 已删除"

echo "  重新执行 cboot（批处理模式，重新 gen）..."
run_cboot
echo "  cboot 退出码: $?"

# 验证重新生成的代码
echo ""
echo "  --- 重新生成的 calc.c 验证 ---"
assert_exists "$CALC_C" "calc.c 已重新生成"
assert_contains "$CALC_C" "calc_div" "重新生成的 calc.c 包含 div 函数"
assert_contains "$CALC_C" "MAX_VALUE 200" "重新生成的 calc.c 宏 MAX_VALUE 为 200"
assert_contains "$CALC_C" "int z;" "重新生成的 calc.c 包含 Point 成员 z"
assert_not_contains "$CALC_C" "calc_mul" "重新生成的 calc.c 不包含 mul 函数"
assert_contains "$CALC_C" "double calc_add" "重新生成的 calc.c add 返回类型为 double"

echo ""
echo "  --- 重新生成的 calc.h 验证 ---"
assert_exists "$CALC_H" "calc.h 已重新生成"
assert_contains "$CALC_H" "calc_div" "重新生成的 calc.h 声明 div 函数"
assert_not_contains "$CALC_H" "calc_mul" "重新生成的 calc.h 不声明 mul 函数"

# ====================================================================
# 测试 7: 测试 adjust 命令（通过批处理脚本模拟交互操作）
# ====================================================================

title "测试 7: 测试 adjust 命令（通过批处理脚本模拟交互操作）"

echo "  重新创建项目用于 adjust 测试..."
rm -rf "$PROJ_DIR"
mkdir -p "$PROJ_DIR"
cd "$PROJ_DIR"
run_cboot "$TEST_DIR/create.cboot"

# 创建 adjust 模拟脚本（通过批处理脚本模拟 adjust 的交互式操作）
# adjust 命令的工作流程: update（同步源码）→ REPL（浏览/修改/生成）
# 此脚本模拟 REPL 阶段的交互操作: 浏览域树 → 修改注释 → 重新生成
cat > "$PROJ_DIR/adjust_sim.cboot" << 'ADJEOF'
# adjust 模拟脚本 - 模拟 adjust 模式下的交互操作
# 1. 加载项目（不含 gen，避免覆盖源码）
# 2. 浏览域树（ls / find）
# 3. 修改函数注释（cd / cmt）
# 4. 重新生成代码（gen）

project calc_project
cmt "测试项目 - 验证 cboot update 和 adjust 功能"

calc/.cboot

# 模拟交互操作: 浏览 calc 模块内容
cd calc
ls

# 模拟交互操作: 查找 add 函数
find void add

# 模拟交互操作: 进入 add 函数，修改注释
cd add
cmt "两数相加 - 已通过 adjust 模拟修改"
cd ..

# 模拟交互操作: 进入 Point 结构体，修改注释
cd Point
cmt "二维坐标点 - 已通过 adjust 模拟修改"
cd ..

# 模拟交互操作: 修改 MAX_VALUE 宏的注释
cd MAX_VALUE
cmt "最大值常量 - 已通过 adjust 模拟修改"
cd ..

# 返回项目根
cd ..

# 重新生成代码
gen
ADJEOF

echo "  执行 adjust 模拟脚本..."
run_cboot adjust_sim.cboot
echo "  cboot 退出码: $?"

echo ""
echo "  --- adjust 模拟操作结果验证 ---"

# 验证注释已通过 adjust 模拟操作修改
assert_contains "$CALC_CBOOT" "已通过 adjust 模拟修改" ".cboot 包含 adjust 修改的注释"
assert_contains "$CALC_CBOOT" "二维坐标点" ".cboot Point 注释已修改"
assert_contains "$CALC_CBOOT" "最大值常量 - 已通过 adjust" ".cboot MAX_VALUE 注释已修改"

# 验证 calc.h 包含修改后的注释
assert_contains "$CALC_H" "已通过 adjust 模拟修改" "calc.h 包含 adjust 修改的注释"

# 验证 gen 已执行（代码重新生成）
assert_exists "$CALC_C" "adjust 后 calc.c 已重新生成"
assert_exists "$CALC_H" "adjust 后 calc.h 已重新生成"

# 验证原始代码内容保持不变（adjust 的 update 无外部修改，代码应保持初始状态）
assert_contains "$CALC_C" "calc_add" "adjust 后 calc.c 仍包含 add 函数"
assert_contains "$CALC_C" "calc_sub" "adjust 后 calc.c 仍包含 sub 函数"
assert_contains "$CALC_C" "calc_mul" "adjust 后 calc.c 仍包含 mul 函数"

# ====================================================================
# 测试结果汇总
# ====================================================================

title "测试结果汇总"

TOTAL=$((PASS + FAIL))
echo "  总计: $TOTAL  通过: $PASS  失败: $FAIL"
echo ""

if [ "$FAIL" -eq 0 ]; then
    echo "  ========================================"
    echo "  所有测试通过!"
    echo "  ========================================"
else
    echo "  ========================================"
    echo "  有 $FAIL 项测试失败!"
    echo "  ========================================"
fi

# ====================================================================
# 清理
# ====================================================================

cd /
rm -rf "$WORK_DIR"

echo ""
echo "  工作目录已清理: $WORK_DIR"

if [ "$FAIL" -eq 0 ]; then
    exit 0
else
    exit 1
fi
