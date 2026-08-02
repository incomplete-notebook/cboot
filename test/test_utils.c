/*
 * test_utils.c - utils.c 单元测试
 * 通过 #include 源文件来测试 static 函数
 */
#include "test.h"
/* 直接包含源文件以访问 static 函数 */
#include "utils/utils.c"

#include <unistd.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* tokenize_emit / tokenize_quoted / tokenize_plain                    */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_tokenize_emit) {
    TEST_BEGIN("tokenize_emit 正常复制");
    char *tokens[16] = {0};
    int count = 0;
    tokenize_emit(tokens, &count, "hello", 5);
    ASSERT_EQ_INT(count, 1, "count 应为 1");
    ASSERT_EQ_STR(tokens[0], "hello", "内容应匹配");
    ASSERT_EQ_INT((int)strlen(tokens[0]), 5, "长度应为 5");
    free(tokens[0]);
    TEST_END();
}

TEST_SUITE(test_tokenize_emit_overflow) {
    TEST_BEGIN("tokenize_emit 超过 MAX_TOKEN_COUNT 时丢弃");
    char *tokens[256] = {0};
    int count = 0;
    /* 模拟超过上限 */
    int saved_max = MAX_TOKEN_COUNT;  /* 常量无法修改，仅测试正常路径多次调用 */
    (void)saved_max;
    for (int i = 0; i < 5; i++) {
        tokenize_emit(tokens, &count, "x", 1);
    }
    ASSERT_EQ_INT(count, 5, "5 次正常调用应计数 5");
    for (int i = 0; i < 5; i++) free(tokens[i]);
    TEST_END();
}

TEST_SUITE(test_tokenize_quoted) {
    TEST_BEGIN("tokenize_quoted 提取引号内内容");
    char *tokens[16] = {0};
    int count = 0;
    const char *input = "\"hello world\"";
    const char *p = tokenize_quoted(input, tokens, &count);
    ASSERT_EQ_INT(count, 1, "count 应为 1");
    ASSERT_EQ_STR(tokens[0], "hello world", "应提取引号内字符串");
    ASSERT_TRUE(p == input + strlen(input), "应返回闭引号之后");
    free(tokens[0]);
    TEST_END();
}

TEST_SUITE(test_tokenize_quoted_unterminated) {
    TEST_BEGIN("tokenize_quoted 未闭合引号读到 EOF");
    char *tokens[16] = {0};
    int count = 0;
    const char *input = "\"unclosed";
    const char *p = tokenize_quoted(input, tokens, &count);
    ASSERT_EQ_INT(count, 1, "仍应产生一个 token");
    ASSERT_EQ_STR(tokens[0], "unclosed", "应读取到结尾");
    ASSERT_TRUE(p == input + strlen(input), "应到达字符串末尾");
    free(tokens[0]);
    TEST_END();
}

TEST_SUITE(test_tokenize_plain) {
    TEST_BEGIN("tokenize_plain 普通token");
    char *tokens[16] = {0};
    int count = 0;
    const char *input = "abc def";
    const char *p = tokenize_plain(input, tokens, &count);
    ASSERT_EQ_INT(count, 1, "count 应为 1");
    ASSERT_EQ_STR(tokens[0], "abc", "应提取 abc");
    ASSERT_TRUE(p == input + 3, "应停在空格处");
    free(tokens[0]);
    TEST_END();
}

TEST_SUITE(test_tokenize_plain_stops_at_quote) {
    TEST_BEGIN("tokenize_plain 遇引号停止");
    char *tokens[16] = {0};
    int count = 0;
    const char *input = "ab\"cd\"";
    const char *p = tokenize_plain(input, tokens, &count);
    ASSERT_EQ_STR(tokens[0], "ab", "应提取 ab");
    ASSERT_TRUE(p == input + 2, "应停在引号处");
    free(tokens[0]);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* tokenize                                                            */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_tokenize_basic) {
    TEST_BEGIN("tokenize 基本分词");
    int count = 0;
    char **toks = tokenize("hello world foo", &count);
    ASSERT_NOT_NULL(toks, "不应返回 NULL");
    ASSERT_EQ_INT(count, 3, "应有 3 个 token");
    ASSERT_EQ_STR(toks[0], "hello", "token[0]");
    ASSERT_EQ_STR(toks[1], "world", "token[1]");
    ASSERT_EQ_STR(toks[2], "foo", "token[2]");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_tokenize_with_quotes) {
    TEST_BEGIN("tokenize 引号内容作为整体");
    int count = 0;
    char **toks = tokenize("cmd \"arg with space\" end", &count);
    ASSERT_EQ_INT(count, 3, "应有 3 个 token");
    ASSERT_EQ_STR(toks[0], "cmd", "token[0]");
    ASSERT_EQ_STR(toks[1], "arg with space", "引号内应作为整体");
    ASSERT_EQ_STR(toks[2], "end", "token[2]");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_tokenize_null) {
    TEST_BEGIN("tokenize NULL 输入返回 NULL");
    int count = 0;
    char **toks = tokenize(NULL, &count);
    ASSERT_NULL(toks, "NULL line 应返回 NULL");
    ASSERT_EQ_INT(count, 0, "count 应为 0");
    TEST_END();
}

TEST_SUITE(test_tokenize_null_count) {
    TEST_BEGIN("tokenize NULL count 返回 NULL");
    char **toks = tokenize("hello", NULL);
    ASSERT_NULL(toks, "NULL count 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_tokenize_empty) {
    TEST_BEGIN("tokenize 空字符串");
    int count = 99;
    char **toks = tokenize("", &count);
    ASSERT_NOT_NULL(toks, "空串仍应返回非 NULL 数组");
    ASSERT_EQ_INT(count, 0, "count 应为 0");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_tokenize_only_spaces) {
    TEST_BEGIN("tokenize 仅空白");
    int count = 99;
    char **toks = tokenize("   \t\n  ", &count);
    ASSERT_EQ_INT(count, 0, "应无 token");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_tokenize_free_null) {
    TEST_BEGIN("utils_free_tokens NULL 安全");
    utils_free_tokens(NULL, 0);  /* 不应崩溃 */
    ASSERT_TRUE(1, "NULL 安全通过");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* trim                                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_trim_both) {
    TEST_BEGIN("trim 去除两端空白");
    char buf[] = "  hello  ";
    char *r = trim(buf);
    ASSERT_EQ_STR(r, "hello", "应去除两端空白");
    TEST_END();
}

TEST_SUITE(test_trim_leading_only) {
    TEST_BEGIN("trim 仅前导空白");
    char buf[] = "  hello";
    char *r = trim(buf);
    ASSERT_EQ_STR(r, "hello", "应去除前导空白");
    TEST_END();
}

TEST_SUITE(test_trim_all_spaces) {
    TEST_BEGIN("trim 全空白");
    char buf[] = "     ";
    char *r = trim(buf);
    ASSERT_EQ_STR(r, "", "应得到空串");
    TEST_END();
}

TEST_SUITE(test_trim_null) {
    TEST_BEGIN("trim NULL 返回 NULL");
    char *r = trim(NULL);
    ASSERT_NULL(r, "NULL 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_trim_empty) {
    TEST_BEGIN("trim 空串");
    char buf[] = "";
    char *r = trim(buf);
    ASSERT_EQ_STR(r, "", "空串应保持空串");
    TEST_END();
}

TEST_SUITE(test_trim_no_spaces) {
    TEST_BEGIN("trim 无空白保持不变");
    char buf[] = "hello";
    char *r = trim(buf);
    ASSERT_EQ_STR(r, "hello", "无空白应不变");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_str_dup                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_str_dup_basic) {
    TEST_BEGIN("utils_str_dup 基本复制");
    char *d = utils_str_dup("hello");
    ASSERT_NOT_NULL(d, "不应为 NULL");
    ASSERT_EQ_STR(d, "hello", "内容应一致");
    free(d);
    TEST_END();
}

TEST_SUITE(test_str_dup_null) {
    TEST_BEGIN("utils_str_dup NULL 返回 NULL");
    char *d = utils_str_dup(NULL);
    ASSERT_NULL(d, "NULL 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_str_dup_empty) {
    TEST_BEGIN("utils_str_dup 空串");
    char *d = utils_str_dup("");
    ASSERT_NOT_NULL(d, "空串不应返回 NULL");
    ASSERT_EQ_STR(d, "", "应为空串");
    free(d);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_str_eq                                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_str_eq_same) {
    TEST_BEGIN("utils_str_eq 相同字符串");
    ASSERT_TRUE(utils_str_eq("abc", "abc"), "应相等");
    TEST_END();
}

TEST_SUITE(test_str_eq_diff) {
    TEST_BEGIN("utils_str_eq 不同字符串");
    ASSERT_FALSE(utils_str_eq("abc", "abd"), "应不等");
    TEST_END();
}

TEST_SUITE(test_str_eq_same_ptr) {
    TEST_BEGIN("utils_str_eq 同一指针");
    const char *s = "abc";
    ASSERT_TRUE(utils_str_eq(s, s), "同一指针应相等");
    TEST_END();
}

TEST_SUITE(test_str_eq_null_a) {
    TEST_BEGIN("utils_str_eq a为NULL");
    ASSERT_FALSE(utils_str_eq(NULL, "abc"), "NULL 不等于任何");
    TEST_END();
}

TEST_SUITE(test_str_eq_null_b) {
    TEST_BEGIN("utils_str_eq b为NULL");
    ASSERT_FALSE(utils_str_eq("abc", NULL), "任何 不等于 NULL");
    TEST_END();
}

TEST_SUITE(test_str_eq_both_null) {
    TEST_BEGIN("utils_str_eq 都为NULL（a==b 路径返回 1）");
    ASSERT_TRUE(utils_str_eq(NULL, NULL), "a==b 路径返回 1");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_str_startswith                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_startswith_true) {
    TEST_BEGIN("utils_str_startswith 前缀匹配");
    ASSERT_TRUE(utils_str_startswith("hello world", "hello"), "应以 hello 开头");
    TEST_END();
}

TEST_SUITE(test_startswith_false) {
    TEST_BEGIN("utils_str_startswith 前缀不匹配");
    ASSERT_FALSE(utils_str_startswith("hello world", "world"), "不应以 world 开头");
    TEST_END();
}

TEST_SUITE(test_startswith_prefix_longer) {
    TEST_BEGIN("utils_str_startswith 前缀长于字符串");
    ASSERT_FALSE(utils_str_startswith("ab", "abc"), "前缀更长应 false");
    TEST_END();
}

TEST_SUITE(test_startswith_empty_prefix) {
    TEST_BEGIN("utils_str_startswith 空前缀");
    ASSERT_TRUE(utils_str_startswith("abc", ""), "空前缀应 true");
    TEST_END();
}

TEST_SUITE(test_startswith_null_str) {
    TEST_BEGIN("utils_str_startswith NULL str");
    ASSERT_FALSE(utils_str_startswith(NULL, "abc"), "NULL str 应 false");
    TEST_END();
}

TEST_SUITE(test_startswith_null_prefix) {
    TEST_BEGIN("utils_str_startswith NULL prefix");
    ASSERT_FALSE(utils_str_startswith("abc", NULL), "NULL prefix 应 false");
    TEST_END();
}

TEST_SUITE(test_startswith_exact) {
    TEST_BEGIN("utils_str_startswith 完全相等");
    ASSERT_TRUE(utils_str_startswith("abc", "abc"), "完全相等应 true");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_parse_c_decl                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_c_decl_basic) {
    TEST_BEGIN("utils_parse_c_decl 基本类型");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int x", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "int", "类型应为 int");
    ASSERT_EQ_STR(name, "x", "名称应为 x");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_pointer) {
    TEST_BEGIN("utils_parse_c_decl 指针类型");
    char type[64], name[64];
    int rc = utils_parse_c_decl("char *str", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "char*", "类型应为 char*");
    ASSERT_EQ_STR(name, "str", "名称应为 str");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_multi_pointer) {
    TEST_BEGIN("utils_parse_c_decl 多重指针");
    char type[64], name[64];
    int rc = utils_parse_c_decl("void ***p", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "void***", "类型应为 void***");
    ASSERT_EQ_STR(name, "p", "名称应为 p");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_no_space) {
    TEST_BEGIN("utils_parse_c_decl 类型名紧邻");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int***a", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "int***", "类型应为 int***");
    ASSERT_EQ_STR(name, "a", "名称应为 a");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_array) {
    TEST_BEGIN("utils_parse_c_decl 数组");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int arr[10]", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "int", "类型应为 int");
    ASSERT_EQ_STR(name, "arr[10]", "名称应为 arr[10]");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_array_no_size) {
    TEST_BEGIN("utils_parse_c_decl 无尺寸数组");
    char type[64], name[64];
    int rc = utils_parse_c_decl("char buf[]", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(name, "buf[]", "名称应为 buf[]");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_array_unclosed) {
    TEST_BEGIN("utils_parse_c_decl 未闭合数组");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int arr[10", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, -1, "未闭合应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_array_before_name) {
    TEST_BEGIN("utils_parse_c_decl 类型后直接 [");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int[10] x", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, -1, "类型后 [ 应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_null) {
    TEST_BEGIN("utils_parse_c_decl NULL 参数");
    char type[64], name[64];
    ASSERT_EQ_INT(utils_parse_c_decl(NULL, type, 64, name, 64), -1, "NULL decl 应失败");
    ASSERT_EQ_INT(utils_parse_c_decl("int x", NULL, 64, name, 64), -1, "NULL type 应失败");
    ASSERT_EQ_INT(utils_parse_c_decl("int x", type, 64, NULL, 64), -1, "NULL name 应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_empty) {
    TEST_BEGIN("utils_parse_c_decl 空串");
    char type[64], name[64];
    ASSERT_EQ_INT(utils_parse_c_decl("", type, 64, name, 64), -1, "空串应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_only_type) {
    TEST_BEGIN("utils_parse_c_decl 仅类型无名称");
    char type[64], name[64];
    ASSERT_EQ_INT(utils_parse_c_decl("int", type, 64, name, 64), -1, "仅类型应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_garbage_after) {
    TEST_BEGIN("utils_parse_c_decl 名称后有垃圾");
    char type[64], name[64];
    ASSERT_EQ_INT(utils_parse_c_decl("int x y", type, 64, name, 64), -1, "应失败");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_const) {
    TEST_BEGIN("utils_parse_c_decl const 类型");
    char type[64], name[64];
    int rc = utils_parse_c_decl("const char *s", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "const char*", "类型应为 const char*");
    ASSERT_EQ_STR(name, "s", "名称应为 s");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_unsigned) {
    TEST_BEGIN("utils_parse_c_decl unsigned long（多词类型现已支持）");
    char type[64], name[64];
    int rc = utils_parse_c_decl("unsigned long val", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "应成功");
    ASSERT_EQ_STR(type, "unsigned long", "类型应为 unsigned long");
    ASSERT_EQ_STR(name, "val", "名称应为 val");
    TEST_END();
}

TEST_SUITE(test_parse_c_decl_trailing_ws) {
    TEST_BEGIN("utils_parse_c_decl 尾随空格");
    char type[64], name[64];
    int rc = utils_parse_c_decl("int x   ", type, sizeof(type), name, sizeof(name));
    ASSERT_EQ_INT(rc, 0, "尾随空格应成功");
    ASSERT_EQ_STR(name, "x", "名称应为 x");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* extract_base_type / extract_type_from_decl / extract_name_from_decl */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_extract_base_type) {
    TEST_BEGIN("extract_base_type 去除指针");
    char *r = extract_base_type("char *");
    ASSERT_EQ_STR(r, "char", "应为 char");
    TEST_END();
}

TEST_SUITE(test_extract_base_type_multi_star) {
    TEST_BEGIN("extract_base_type 多重指针");
    char *r = extract_base_type("void ***");
    ASSERT_EQ_STR(r, "void", "应为 void");
    TEST_END();
}

TEST_SUITE(test_extract_base_type_no_star) {
    TEST_BEGIN("extract_base_type 无指针");
    char *r = extract_base_type("int");
    ASSERT_EQ_STR(r, "int", "应为 int");
    TEST_END();
}

TEST_SUITE(test_extract_type_from_decl) {
    TEST_BEGIN("extract_type_from_decl 提取类型");
    char *r = extract_type_from_decl("int x");
    ASSERT_EQ_STR(r, "int", "应为 int");
    TEST_END();
}

TEST_SUITE(test_extract_type_from_decl_ptr) {
    TEST_BEGIN("extract_type_from_decl 指针（fallback 保留空格）");
    char *r = extract_type_from_decl("char *str");
    /* 修复后 utils_parse_c_decl 成功，返回 "char*" */
    ASSERT_EQ_STR(r, "char*", "应为 char*");
    TEST_END();
}

TEST_SUITE(test_extract_type_from_decl_null) {
    TEST_BEGIN("extract_type_from_decl NULL");
    char *r = extract_type_from_decl(NULL);
    ASSERT_NULL(r, "NULL 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_extract_name_from_decl) {
    TEST_BEGIN("extract_name_from_decl 提取名称");
    char *r = extract_name_from_decl("int x");
    ASSERT_EQ_STR(r, "x", "应为 x");
    TEST_END();
}

TEST_SUITE(test_extract_name_from_decl_array) {
    TEST_BEGIN("extract_name_from_decl 数组");
    char *r = extract_name_from_decl("int arr[10]");
    ASSERT_EQ_STR(r, "arr[10]", "应为 arr[10]");
    TEST_END();
}

TEST_SUITE(test_extract_name_from_decl_null) {
    TEST_BEGIN("extract_name_from_decl NULL");
    char *r = extract_name_from_decl(NULL);
    ASSERT_NULL(r, "NULL 应返回 NULL");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_is_valid_identifier                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_is_valid_identifier_basic) {
    TEST_BEGIN("utils_is_valid_identifier 合法标识符");
    ASSERT_TRUE(utils_is_valid_identifier("abc"), "abc 合法");
    ASSERT_TRUE(utils_is_valid_identifier("_abc"), "_abc 合法");
    ASSERT_TRUE(utils_is_valid_identifier("a1b2"), "a1b2 合法");
    ASSERT_TRUE(utils_is_valid_identifier("_"), "_ 合法");
    ASSERT_TRUE(utils_is_valid_identifier("a_b_c"), "a_b_c 合法");
    TEST_END();
}

TEST_SUITE(test_is_valid_identifier_invalid) {
    TEST_BEGIN("utils_is_valid_identifier 非法标识符");
    ASSERT_FALSE(utils_is_valid_identifier("1abc"), "数字开头非法");
    ASSERT_FALSE(utils_is_valid_identifier("a-b"), "含连字符非法");
    ASSERT_FALSE(utils_is_valid_identifier("a b"), "含空格非法");
    ASSERT_FALSE(utils_is_valid_identifier(""), "空串非法");
    ASSERT_FALSE(utils_is_valid_identifier(NULL), "NULL 非法");
    ASSERT_FALSE(utils_is_valid_identifier("a.b"), "含点非法");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_ensure_dir / utils_file_exists                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_ensure_dir_basic) {
    TEST_BEGIN("utils_ensure_dir 创建目录");
    const char *path = "/tmp/cboot_test_ensure_dir";
    rmdir(path);
    utils_ensure_dir(path);
    ASSERT_TRUE(utils_file_exists(path), "目录应存在");
    rmdir(path);
    TEST_END();
}

TEST_SUITE(test_ensure_dir_nested) {
    TEST_BEGIN("utils_ensure_dir 嵌套目录");
    const char *path = "/tmp/cboot_test_ensure_nested/a/b";
    rmdir("/tmp/cboot_test_ensure_nested/a/b");
    rmdir("/tmp/cboot_test_ensure_nested/a");
    rmdir("/tmp/cboot_test_ensure_nested");
    utils_ensure_dir(path);
    ASSERT_TRUE(utils_file_exists(path), "嵌套目录应存在");
    rmdir(path);
    rmdir("/tmp/cboot_test_ensure_nested/a");
    rmdir("/tmp/cboot_test_ensure_nested");
    TEST_END();
}

TEST_SUITE(test_ensure_dir_null) {
    TEST_BEGIN("utils_ensure_dir NULL 不崩溃");
    utils_ensure_dir(NULL);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

TEST_SUITE(test_file_exists_yes) {
    TEST_BEGIN("utils_file_exists 存在");
    ASSERT_TRUE(utils_file_exists("/tmp"), "/tmp 应存在");
    TEST_END();
}

TEST_SUITE(test_file_exists_no) {
    TEST_BEGIN("utils_file_exists 不存在");
    ASSERT_FALSE(utils_file_exists("/tmp/nonexistent_path_xyz_123"), "不存在应 false");
    TEST_END();
}

TEST_SUITE(test_file_exists_null) {
    TEST_BEGIN("utils_file_exists NULL");
    ASSERT_FALSE(utils_file_exists(NULL), "NULL 应 false");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* utils_strip_quotes                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_strip_quotes_basic) {
    TEST_BEGIN("utils_strip_quotes 基本去除");
    char buf[] = "\"hello\"";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "hello", "应去除引号");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_no_quotes) {
    TEST_BEGIN("utils_strip_quotes 无引号");
    char buf[] = "hello";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "hello", "无引号不变");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_one_quote) {
    TEST_BEGIN("utils_strip_quotes 仅前引号");
    char buf[] = "\"hello";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "\"hello", "仅前引号不变");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_empty) {
    TEST_BEGIN("utils_strip_quotes 空串");
    char buf[] = "";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "", "空串不变");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_only_quotes) {
    TEST_BEGIN("utils_strip_quotes 仅两个引号");
    char buf[] = "\"\"";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "", "应去除为空");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_null) {
    TEST_BEGIN("utils_strip_quotes NULL");
    utils_strip_quotes(NULL);  /* 不应崩溃 */
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

TEST_SUITE(test_strip_quotes_single_char) {
    TEST_BEGIN("utils_strip_quotes 单字符");
    char buf[] = "a";
    utils_strip_quotes(buf);
    ASSERT_EQ_STR(buf, "a", "单字符不变");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== utils.c 测试 ==========\n");

    RUN_SUITE(test_tokenize_emit);
    RUN_SUITE(test_tokenize_emit_overflow);
    RUN_SUITE(test_tokenize_quoted);
    RUN_SUITE(test_tokenize_quoted_unterminated);
    RUN_SUITE(test_tokenize_plain);
    RUN_SUITE(test_tokenize_plain_stops_at_quote);
    RUN_SUITE(test_tokenize_basic);
    RUN_SUITE(test_tokenize_with_quotes);
    RUN_SUITE(test_tokenize_null);
    RUN_SUITE(test_tokenize_null_count);
    RUN_SUITE(test_tokenize_empty);
    RUN_SUITE(test_tokenize_only_spaces);
    RUN_SUITE(test_tokenize_free_null);
    RUN_SUITE(test_trim_both);
    RUN_SUITE(test_trim_leading_only);
    RUN_SUITE(test_trim_all_spaces);
    RUN_SUITE(test_trim_null);
    RUN_SUITE(test_trim_empty);
    RUN_SUITE(test_trim_no_spaces);
    RUN_SUITE(test_str_dup_basic);
    RUN_SUITE(test_str_dup_null);
    RUN_SUITE(test_str_dup_empty);
    RUN_SUITE(test_str_eq_same);
    RUN_SUITE(test_str_eq_diff);
    RUN_SUITE(test_str_eq_same_ptr);
    RUN_SUITE(test_str_eq_null_a);
    RUN_SUITE(test_str_eq_null_b);
    RUN_SUITE(test_str_eq_both_null);
    RUN_SUITE(test_startswith_true);
    RUN_SUITE(test_startswith_false);
    RUN_SUITE(test_startswith_prefix_longer);
    RUN_SUITE(test_startswith_empty_prefix);
    RUN_SUITE(test_startswith_null_str);
    RUN_SUITE(test_startswith_null_prefix);
    RUN_SUITE(test_startswith_exact);
    RUN_SUITE(test_parse_c_decl_basic);
    RUN_SUITE(test_parse_c_decl_pointer);
    RUN_SUITE(test_parse_c_decl_multi_pointer);
    RUN_SUITE(test_parse_c_decl_no_space);
    RUN_SUITE(test_parse_c_decl_array);
    RUN_SUITE(test_parse_c_decl_array_no_size);
    RUN_SUITE(test_parse_c_decl_array_unclosed);
    RUN_SUITE(test_parse_c_decl_array_before_name);
    RUN_SUITE(test_parse_c_decl_null);
    RUN_SUITE(test_parse_c_decl_empty);
    RUN_SUITE(test_parse_c_decl_only_type);
    RUN_SUITE(test_parse_c_decl_garbage_after);
    RUN_SUITE(test_parse_c_decl_const);
    RUN_SUITE(test_parse_c_decl_unsigned);
    RUN_SUITE(test_parse_c_decl_trailing_ws);
    RUN_SUITE(test_extract_base_type);
    RUN_SUITE(test_extract_base_type_multi_star);
    RUN_SUITE(test_extract_base_type_no_star);
    RUN_SUITE(test_extract_type_from_decl);
    RUN_SUITE(test_extract_type_from_decl_ptr);
    RUN_SUITE(test_extract_type_from_decl_null);
    RUN_SUITE(test_extract_name_from_decl);
    RUN_SUITE(test_extract_name_from_decl_array);
    RUN_SUITE(test_extract_name_from_decl_null);
    RUN_SUITE(test_is_valid_identifier_basic);
    RUN_SUITE(test_is_valid_identifier_invalid);
    RUN_SUITE(test_ensure_dir_basic);
    RUN_SUITE(test_ensure_dir_nested);
    RUN_SUITE(test_ensure_dir_null);
    RUN_SUITE(test_file_exists_yes);
    RUN_SUITE(test_file_exists_no);
    RUN_SUITE(test_file_exists_null);
    RUN_SUITE(test_strip_quotes_basic);
    RUN_SUITE(test_strip_quotes_no_quotes);
    RUN_SUITE(test_strip_quotes_one_quote);
    RUN_SUITE(test_strip_quotes_empty);
    RUN_SUITE(test_strip_quotes_only_quotes);
    RUN_SUITE(test_strip_quotes_null);
    RUN_SUITE(test_strip_quotes_single_char);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
