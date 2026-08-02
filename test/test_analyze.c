/*
 * test_analyze.c - commands/analyze/analyze.c 单元测试
 *
 * 通过 #include "commands/analyze/analyze.c" 方式包含被测源文件以测试 static 函数。
 * analyze.c 依赖: utils, domain/core, domain, typecheck, commands, analyze
 * 需 mock 的缺失函数: generator_*, docgen_*, parser_parse_cboot_script,
 *                     cupdate_run_project
 *
 * 测试覆盖：
 *   - 行计数状态机 (analyze_count_line_end, analyze_count_normal_char,
 *     analyze_count_quote_char, count_st_*, analyze_count_code_lines)
 *   - 关键字检测 (analyze_is_keyword)
 *   - 跳过非代码 (analyze_skip_line_comment, analyze_skip_block_comment,
 *     analyze_skip_non_code, analyze_skip_string)
 *   - 函数体查找 (analyze_find_body_end)
 *   - 括号匹配 (analyze_find_parens)
 *   - 函数名提取 (analyze_extract_func_name)
 *   - 函数提取 (analyze_try_extract_function, analyze_extract_functions)
 *   - 运算符检测 (analyze_is_two_char_op)
 *   - token 发射 (analyze_emit_literal, analyze_emit_identifier,
 *     analyze_emit_operator, analyze_emit_literal_token)
 *   - 空白跳过 (analyze_skip_trivia)
 *   - 词法分析 (analyze_tokenize)
 *   - 圈复杂度 (analyze_cyclomatic_complexity)
 *   - n-gram 比对 (analyze_ngram_equal, analyze_ngram_match_pattern,
 *     analyze_ngram_is_common, analyze_ngram_is_dup)
 *   - 模块路径 (analyze_build_module_path)
 *   - 模块收集 (analyze_collect_modules)
 *   - 调用检测 (analyze_has_call, analyze_resolve_call)
 *   - 依赖复杂度 (dependency_compute_counts, dependency_report,
 *     commands_analyze_dependency_complexity)
 *   - 重复片段 (analyze_print_first_dup_pair, analyze_print_dup_fragments,
 *     commands_analyze_duplication)
 *   - 汇总打印 (commands_analyze_code_lines, commands_analyze_cyclomatic)
 *   - 加载检查 (commands_analyze_ensure_loaded)
 *   - 公共入口 (commands_cmd_analyze_impl)
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "commands/commands.c"
#include "commands/analyze/analyze.c"

#include <unistd.h>
#include <fcntl.h>

/* ------------------------------------------------------------------ */
/* 全局状态（cboot.h 中 extern 声明，此处定义）                        */
/* ------------------------------------------------------------------ */
Project *g_proj     = NULL;
RunMode  g_mode     = MODE_INTERACTIVE;
int      g_force    = 0;
int      g_running  = 1;
int      g_skip_gen = 0;
char     g_script_dir[MAX_PATH_LEN] = ".";

/* ------------------------------------------------------------------ */
/* mock: 外部模块函数                                                  */
/* ------------------------------------------------------------------ */
static int g_mock_parser_return = 0;
static int g_mock_parser_calls  = 0;

int  generator_generate_project(Project *proj)      { (void)proj; return 0; }
int  generator_generate_cboot_only(Project *proj)    { (void)proj; return 0; }
int  docgen_generate_docs(Project *proj, const char *dir) { (void)proj; (void)dir; return 0; }
int  parser_parse_cboot_script(const char *filename) { (void)filename; g_mock_parser_calls++; return g_mock_parser_return; }
int  cupdate_run_project(Project *proj, int *err_cnt, int *warn_cnt) {
    (void)proj; if (err_cnt) *err_cnt = 0; if (warn_cnt) *warn_cnt = 0; return 0;
}

/* ------------------------------------------------------------------ */
/* 测试辅助                                                            */
/* ------------------------------------------------------------------ */
#define CASE(name)   TEST_BEGIN(name); {
#define ENDCASE      TEST_END(); }

static char g_orig_cwd[MAX_PATH_LEN];
static int  g_saved_stdout = -1;

static void reset_proj(void) {
    if (g_proj) domain_project_free(g_proj);
    g_proj = domain_project_new("test");
    g_proj->has_generated = 0;
}

static void suppress_stdout(void) {
    fflush(stdout);
    g_saved_stdout = dup(STDOUT_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) { dup2(devnull, STDOUT_FILENO); close(devnull); }
}
static void restore_stdout(void) {
    if (g_saved_stdout >= 0) {
        fflush(stdout);
        dup2(g_saved_stdout, STDOUT_FILENO);
        close(g_saved_stdout);
        g_saved_stdout = -1;
    }
}

static int write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) sz = 0;
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static void make_tmp_dir(const char *path) {
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'", path, path);
    system(cmd);
}
static void enter_tmp(const char *path) {
    make_tmp_dir(path);
    chdir(path);
}
static void leave_tmp(const char *path) {
    chdir(g_orig_cwd);
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);
}

/* 构造 AnalyzeToken */
static AnalyzeToken make_tok(const char *text) {
    AnalyzeToken t;
    strncpy(t.text, text, 63);
    t.text[63] = '\0';
    return t;
}

/* 填充 8-token n-gram */
static void fill_gram(AnalyzeToken *g, const char *t0, const char *t1,
                      const char *t2, const char *t3, const char *t4,
                      const char *t5, const char *t6, const char *t7) {
    g[0] = make_tok(t0); g[1] = make_tok(t1); g[2] = make_tok(t2);
    g[3] = make_tok(t3); g[4] = make_tok(t4); g[5] = make_tok(t5);
    g[6] = make_tok(t6); g[7] = make_tok(t7);
}

/* 添加带 code 的 SRC 模块到 root */
static ModuleDomain *add_mod(const char *name, const char *code) {
    ModuleDomain *m = domain_module_domain_new(name);
    if (code) domain_domain_set_code((Domain *)m, code);
    domain_domain_add_child(g_proj->root, (Domain *)m);
    return m;
}

/* ================================================================== */
/* 1. analyze_count_line_end                                          */
/* ================================================================== */
TEST_SUITE(test_count_line_end) {
    CASE("count_line_end: has_code=1 时 lines++");
    int has_code = 1, lines = 0;
    analyze_count_line_end(&has_code, &lines);
    ASSERT_EQ_INT(lines, 1, "lines 应 +1");
    ASSERT_EQ_INT(has_code, 0, "has_code 重置为 0");
    ENDCASE;

    CASE("count_line_end: has_code=0 时 lines 不变");
    int has_code2 = 0, lines2 = 5;
    analyze_count_line_end(&has_code2, &lines2);
    ASSERT_EQ_INT(lines2, 5, "lines 不变");
    ASSERT_EQ_INT(has_code2, 0, "has_code 仍 0");
    ENDCASE;
}

/* ================================================================== */
/* 2. analyze_count_normal_char                                       */
/* ================================================================== */
TEST_SUITE(test_count_normal_char) {
    CASE("normal_char: // 进入行注释");
    {
        const char *p = "// comment";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char('/', p, &next, &has_code);
        ASSERT_EQ_INT(s, 1, "返回 ST_LINE_CMT");
        ASSERT_TRUE(next > p, "next 前进");
        ENDCASE;
    }

    CASE("normal_char: /* 进入块注释");
    {
        const char *p = "/* c */";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char('/', p, &next, &has_code);
        ASSERT_EQ_INT(s, 2, "返回 ST_BLOCK_CMT");
        ASSERT_TRUE(next > p, "next 前进");
        ENDCASE;
    }

    CASE("normal_char: \" 进入字符串");
    {
        const char *p = "\"str\"";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char('"', p, &next, &has_code);
        ASSERT_EQ_INT(s, 3, "返回 ST_STRING");
        ASSERT_EQ_INT(has_code, 1, "has_code=1");
        ENDCASE;
    }

    CASE("normal_char: ' 进入字符");
    {
        const char *p = "'c'";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char('\'', p, &next, &has_code);
        ASSERT_EQ_INT(s, 4, "返回 ST_CHAR");
        ASSERT_EQ_INT(has_code, 1, "has_code=1");
        ENDCASE;
    }

    CASE("normal_char: 普通字符设 has_code");
    {
        const char *p = "x";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char('x', p, &next, &has_code);
        ASSERT_EQ_INT(s, 0, "返回 ST_NORMAL");
        ASSERT_EQ_INT(has_code, 1, "has_code=1");
        ENDCASE;
    }

    CASE("normal_char: 空格不设 has_code");
    {
        const char *p = " ";
        const char *next = p;
        int has_code = 0;
        int s = analyze_count_normal_char(' ', p, &next, &has_code);
        ASSERT_EQ_INT(s, 0, "返回 ST_NORMAL");
        ASSERT_EQ_INT(has_code, 0, "has_code 不变");
        ENDCASE;
    }
}

/* ================================================================== */
/* 3. analyze_count_quote_char                                        */
/* ================================================================== */
TEST_SUITE(test_count_quote_char) {
    CASE("quote_char: 反斜杠跳过下一字符");
    {
        const char *p = "x";
        const char *next = p;
        int s = analyze_count_quote_char('\\', '"', &next);
        ASSERT_EQ_INT(s, 1, "保持在引号状态");
        ASSERT_TRUE(next > p, "next 前进 1");
        ENDCASE;
    }

    CASE("quote_char: 匹配引号退出");
    {
        const char *p = "x";
        const char *next = p;
        int s = analyze_count_quote_char('"', '"', &next);
        ASSERT_EQ_INT(s, 0, "退出引号状态");
        ASSERT_TRUE(next == p, "next 不变");
        ENDCASE;
    }

    CASE("quote_char: 其他字符保持引号");
    {
        const char *p = "x";
        const char *next = p;
        int s = analyze_count_quote_char('a', '"', &next);
        ASSERT_EQ_INT(s, 1, "保持引号状态");
        ASSERT_TRUE(next == p, "next 不变");
        ENDCASE;
    }
}

/* ================================================================== */
/* 4. count_st_normal / count_st_line_cmt / count_st_block_cmt /      */
/*    count_st_quote                                                   */
/* ================================================================== */
TEST_SUITE(test_count_states) {
    CASE("count_st_normal: 换行触发 line_end");
    {
        CountCtx ctx = {0, 1, 0, 0};
        const char *next;
        int s = count_st_normal('\n', "\n", &next, &ctx);
        ASSERT_EQ_INT(s, 0, "返回 NORMAL");
        ASSERT_EQ_INT(ctx.lines, 1, "lines++");
        ASSERT_EQ_INT(ctx.has_code, 0, "has_code 重置");
        ENDCASE;
    }

    CASE("count_st_normal: // 进入 LINE_CMT");
    {
        CountCtx ctx = {0, 0, 0, 0};
        const char *p = "// x";
        const char *next = p;
        int s = count_st_normal('/', p, &next, &ctx);
        ASSERT_EQ_INT(s, 1, "返回 LINE_CMT");
        ENDCASE;
    }

    CASE("count_st_normal: /* 进入 BLOCK_CMT");
    {
        CountCtx ctx = {0, 0, 0, 0};
        const char *p = "/* x";
        const char *next = p;
        int s = count_st_normal('/', p, &next, &ctx);
        ASSERT_EQ_INT(s, 2, "返回 BLOCK_CMT");
        ENDCASE;
    }

    CASE("count_st_normal: \" 进入 QUOTE 设 quote='\"'");
    {
        CountCtx ctx = {0, 0, 0, 0};
        const char *p = "\"x";
        const char *next = p;
        int s = count_st_normal('"', p, &next, &ctx);
        ASSERT_EQ_INT(s, 3, "返回 QUOTE");
        ASSERT_EQ_INT(ctx.quote, '"', "quote='\"'");
        ENDCASE;
    }

    CASE("count_st_normal: ' 进入 QUOTE 设 quote='\\''");
    {
        CountCtx ctx = {0, 0, 0, 0};
        const char *p = "'x";
        const char *next = p;
        int s = count_st_normal('\'', p, &next, &ctx);
        ASSERT_EQ_INT(s, 3, "返回 QUOTE");
        ASSERT_EQ_INT(ctx.quote, '\'', "quote='\\''");
        ENDCASE;
    }

    CASE("count_st_line_cmt: 换行退出");
    {
        CountCtx ctx = {0, 1, 1, 0};
        int s = count_st_line_cmt('\n', &ctx);
        ASSERT_EQ_INT(s, 0, "返回 NORMAL");
        ASSERT_EQ_INT(ctx.lines, 1, "lines++");
        ENDCASE;
    }

    CASE("count_st_line_cmt: 其他保持");
    {
        CountCtx ctx = {0, 0, 1, 0};
        int s = count_st_line_cmt('x', &ctx);
        ASSERT_EQ_INT(s, 1, "保持 LINE_CMT");
        ENDCASE;
    }

    CASE("count_st_block_cmt: */ 退出");
    {
        CountCtx ctx = {0, 0, 2, 0};
        const char *p = "*/";
        const char *next = p;
        int s = count_st_block_cmt('*', p, &next, &ctx);
        ASSERT_EQ_INT(s, 0, "返回 NORMAL");
        ASSERT_TRUE(next > p, "next 前进");
        ENDCASE;
    }

    CASE("count_st_block_cmt: 换行触发 line_end");
    {
        CountCtx ctx = {0, 1, 2, 0};
        const char *p = "\nx";
        const char *next = p;
        int s = count_st_block_cmt('\n', p, &next, &ctx);
        ASSERT_EQ_INT(s, 2, "保持 BLOCK_CMT");
        ASSERT_EQ_INT(ctx.lines, 1, "lines++");
        ENDCASE;
    }

    CASE("count_st_block_cmt: 其他保持");
    {
        CountCtx ctx = {0, 0, 2, 0};
        const char *p = "x";
        const char *next = p;
        int s = count_st_block_cmt('x', p, &next, &ctx);
        ASSERT_EQ_INT(s, 2, "保持 BLOCK_CMT");
        ENDCASE;
    }

    CASE("count_st_quote: 匹配引号退出");
    {
        CountCtx ctx = {0, 0, 3, '"'};
        const char *next;
        int s = count_st_quote('"', &next, &ctx);
        ASSERT_EQ_INT(s, 0, "返回 NORMAL");
        ENDCASE;
    }

    CASE("count_st_quote: 反斜杠跳过");
    {
        CountCtx ctx = {0, 0, 3, '"'};
        const char *p = "x";
        const char *next = p;
        int s = count_st_quote('\\', &next, &ctx);
        ASSERT_EQ_INT(s, 3, "保持 QUOTE");
        ASSERT_TRUE(next > p, "next 前进");
        ENDCASE;
    }

    CASE("count_st_quote: 换行触发 line_end");
    {
        CountCtx ctx = {5, 1, 3, '"'};
        const char *next;
        int s = count_st_quote('\n', &next, &ctx);
        ASSERT_EQ_INT(s, 3, "保持 QUOTE");
        ASSERT_EQ_INT(ctx.lines, 6, "lines++");
        ENDCASE;
    }
}

/* ================================================================== */
/* 5. analyze_count_code_lines                                        */
/* ================================================================== */
TEST_SUITE(test_count_code_lines) {
    CASE("count_code_lines: NULL 返回 0");
    ASSERT_EQ_INT(analyze_count_code_lines(NULL), 0, "NULL");
    ENDCASE;

    CASE("count_code_lines: 空串返回 0");
    ASSERT_EQ_INT(analyze_count_code_lines(""), 0, "空串");
    ENDCASE;

    CASE("count_code_lines: 单行无换行");
    ASSERT_EQ_INT(analyze_count_code_lines("int x;"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 单行有换行");
    ASSERT_EQ_INT(analyze_count_code_lines("int x;\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 多行代码");
    ASSERT_EQ_INT(analyze_count_code_lines("int x;\nint y;\nint z;\n"), 3, "3 行");
    ENDCASE;

    CASE("count_code_lines: 纯注释行不计");
    ASSERT_EQ_INT(analyze_count_code_lines("// comment\n"), 0, "0 行");
    ENDCASE;

    CASE("count_code_lines: 块注释不计");
    ASSERT_EQ_INT(analyze_count_code_lines("/* comment */\n"), 0, "0 行");
    ENDCASE;

    CASE("count_code_lines: 多行块注释");
    ASSERT_EQ_INT(analyze_count_code_lines("/* line1\nline2\nline3 */\n"), 0, "0 行");
    ENDCASE;

    CASE("count_code_lines: 代码+行注释");
    ASSERT_EQ_INT(analyze_count_code_lines("int x; // comment\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 代码+块注释");
    ASSERT_EQ_INT(analyze_count_code_lines("int x; /* c */\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 空行不计");
    ASSERT_EQ_INT(analyze_count_code_lines("\n\n\n"), 0, "0 行");
    ENDCASE;

    CASE("count_code_lines: 混合空行和代码");
    ASSERT_EQ_INT(analyze_count_code_lines("\nint x;\n\nint y;\n"), 2, "2 行");
    ENDCASE;

    CASE("count_code_lines: 字符串中的大括号");
    ASSERT_EQ_INT(analyze_count_code_lines("char c = '{';\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 字符串中的引号转义");
    ASSERT_EQ_INT(analyze_count_code_lines("char *s = \"hello\\\"world\";\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 字符串跨行不终止");
    /* 字符串中遇到 \n 会触发 line_end（has_code=1 → lines++），
     * 但仍在 QUOTE 状态。后续字符仍在引号中。 */
    ASSERT_EQ_INT(analyze_count_code_lines("\"line1\nline2\";\n"), 2, "2 行（字符串内换行也计数）");
    ENDCASE;

    CASE("count_code_lines: 字符字面量");
    ASSERT_EQ_INT(analyze_count_code_lines("char c = '\\n';\n"), 1, "1 行");
    ENDCASE;

    CASE("count_code_lines: 预处理指令计为代码");
    /* # 不在计数状态机中特殊处理，计为普通代码字符 */
    ASSERT_EQ_INT(analyze_count_code_lines("#include <stdio.h>\n"), 1, "1 行");
    ENDCASE;
}

/* ================================================================== */
/* 6. analyze_is_keyword                                              */
/* ================================================================== */
TEST_SUITE(test_is_keyword) {
    CASE("is_keyword: 各控制流关键字");
    ASSERT_EQ_INT(analyze_is_keyword("if", 2), 1, "if");
    ASSERT_EQ_INT(analyze_is_keyword("for", 3), 1, "for");
    ASSERT_EQ_INT(analyze_is_keyword("while", 5), 1, "while");
    ASSERT_EQ_INT(analyze_is_keyword("switch", 6), 1, "switch");
    ASSERT_EQ_INT(analyze_is_keyword("return", 6), 1, "return");
    ASSERT_EQ_INT(analyze_is_keyword("sizeof", 6), 1, "sizeof");
    ASSERT_EQ_INT(analyze_is_keyword("else", 4), 1, "else");
    ASSERT_EQ_INT(analyze_is_keyword("do", 2), 1, "do");
    ENDCASE;

    CASE("is_keyword: 非关键字");
    ASSERT_EQ_INT(analyze_is_keyword("main", 4), 0, "main");
    ASSERT_EQ_INT(analyze_is_keyword("func", 4), 0, "func");
    ASSERT_EQ_INT(analyze_is_keyword("x", 1), 0, "x");
    ENDCASE;

    CASE("is_keyword: 长度不匹配");
    /* "iff" 长度 3 != 2, 不匹配 "if" */
    ASSERT_EQ_INT(analyze_is_keyword("iff", 3), 0, "iff 不匹配 if");
    ENDCASE;
}

/* ================================================================== */
/* 7. analyze_skip_line_comment / analyze_skip_block_comment /        */
/*    analyze_skip_non_code / analyze_skip_string                     */
/* ================================================================== */
TEST_SUITE(test_skip_functions) {
    CASE("skip_line_comment: 读到换行");
    {
        const char *p = "// hello\nworld";
        const char *r = analyze_skip_line_comment(p);
        ASSERT_TRUE(*r == '\n', "指向换行");
        ENDCASE;
    }

    CASE("skip_line_comment: 读到 EOF");
    {
        const char *p = "// hello";
        const char *r = analyze_skip_line_comment(p);
        ASSERT_TRUE(*r == '\0', "指向 EOF");
        ENDCASE;
    }

    CASE("skip_block_comment: 正常终止");
    {
        const char *p = "/* hello */x";
        const char *r = analyze_skip_block_comment(p);
        ASSERT_EQ_INT(*r, 'x', "指向 */ 后的字符");
        ENDCASE;
    }

    CASE("skip_block_comment: 未终止");
    {
        const char *p = "/* hello";
        const char *r = analyze_skip_block_comment(p);
        ASSERT_TRUE(*r == '\0', "指向 EOF");
        ENDCASE;
    }

    CASE("skip_non_code: 字符串");
    {
        const char *p = "\"hello\"x";
        const char *r = analyze_skip_non_code(p);
        ASSERT_EQ_INT(*r, 'x', "指向字符串后的字符");
        ENDCASE;
    }

    CASE("skip_non_code: 字符字面量");
    {
        const char *p = "'c'x";
        const char *r = analyze_skip_non_code(p);
        ASSERT_EQ_INT(*r, 'x', "指向字符后的字符");
        ENDCASE;
    }

    CASE("skip_non_code: 行注释");
    {
        const char *p = "// c\nx";
        const char *r = analyze_skip_non_code(p);
        ASSERT_EQ_INT(*r, '\n', "指向换行");
        ENDCASE;
    }

    CASE("skip_non_code: 预处理指令");
    {
        const char *p = "#define X\nx";
        const char *r = analyze_skip_non_code(p);
        ASSERT_EQ_INT(*r, '\n', "指向换行");
        ENDCASE;
    }

    CASE("skip_non_code: 块注释");
    {
        const char *p = "/* c */x";
        const char *r = analyze_skip_non_code(p);
        ASSERT_EQ_INT(*r, 'x', "指向块注释后的字符");
        ENDCASE;
    }

    CASE("skip_non_code: 普通字符返回 NULL");
    {
        const char *p = "int x;";
        const char *r = analyze_skip_non_code(p);
        ASSERT_NULL(r, "普通字符返回 NULL");
        ENDCASE;
    }

    CASE("skip_string: 正常字符串");
    {
        const char *p = "\"hello\"x";
        const char *r = analyze_skip_string(p, '"');
        ASSERT_EQ_INT(*r, 'x', "指向字符串后");
        ENDCASE;
    }

    CASE("skip_string: 转义引号");
    {
        const char *p = "\"he\\\"llo\"x";
        const char *r = analyze_skip_string(p, '"');
        ASSERT_EQ_INT(*r, 'x', "跳过转义引号");
        ENDCASE;
    }

    CASE("skip_string: 未终止");
    {
        const char *p = "\"hello";
        const char *r = analyze_skip_string(p, '"');
        ASSERT_TRUE(*r == '\0', "指向 EOF");
        ENDCASE;
    }

    CASE("skip_string: 空字符串");
    {
        const char *p = "\"\"x";
        const char *r = analyze_skip_string(p, '"');
        ASSERT_EQ_INT(*r, 'x', "指向空字符串后");
        ENDCASE;
    }
}

/* ================================================================== */
/* 8. analyze_find_body_end                                           */
/* ================================================================== */
TEST_SUITE(test_find_body_end) {
    CASE("find_body_end: 简单体");
    {
        const char *body = "} x";
        const char *r = analyze_find_body_end(body);
        ASSERT_NOT_NULL(r, "应找到结束");
        ASSERT_EQ_INT(*r, ' ', "指向 } 后");
        ENDCASE;
    }

    CASE("find_body_end: 嵌套大括号");
    {
        const char *body = "{ if (x) {} } } rest";
        const char *r = analyze_find_body_end(body);
        ASSERT_NOT_NULL(r, "应找到结束");
        ASSERT_EQ_INT(*r, ' ', "指向最外层 } 后");
        ENDCASE;
    }

    CASE("find_body_end: 字符串中的大括号不计");
    {
        const char *body = "\"{\"} rest";
        const char *r = analyze_find_body_end(body);
        ASSERT_NOT_NULL(r, "应找到结束");
        ASSERT_EQ_INT(*r, ' ', "指向 } 后（跳过字符串中的 {）");
        ENDCASE;
    }

    CASE("find_body_end: 注释中的大括号不计");
    {
        const char *body = "/* { */} rest";
        const char *r = analyze_find_body_end(body);
        ASSERT_NOT_NULL(r, "应找到结束");
        ASSERT_EQ_INT(*r, ' ', "指向 } 后（跳过注释中的 {）");
        ENDCASE;
    }

    CASE("find_body_end: 未闭合返回 NULL");
    {
        const char *body = "return 0;";
        const char *r = analyze_find_body_end(body);
        ASSERT_NULL(r, "未闭合返回 NULL");
        ENDCASE;
    }

    CASE("find_body_end: 空体");
    {
        const char *body = "}";
        const char *r = analyze_find_body_end(body);
        ASSERT_NOT_NULL(r, "应找到结束");
        ENDCASE;
    }
}

/* ================================================================== */
/* 9. analyze_find_parens                                             */
/* ================================================================== */
TEST_SUITE(test_find_parens) {
    CASE("find_parens: f() { 正常");
    {
        const char *open, *close;
        int r = analyze_find_parens("f() {", &open, &close);
        ASSERT_EQ_INT(r, 1, "应成功");
        ASSERT_EQ_INT(*open, '(', "open 指向 (");
        ASSERT_EQ_INT(*close, ')', "close 指向 )");
        ENDCASE;
    }

    CASE("find_parens: f(); 分号停止");
    {
        const char *open, *close;
        int r = analyze_find_parens("f();", &open, &close);
        ASSERT_EQ_INT(r, 0, "分号停止返回 0");
        ENDCASE;
    }

    CASE("find_parens: 嵌套括号");
    {
        const char *open, *close;
        int r = analyze_find_parens("f(a, (b)) {", &open, &close);
        ASSERT_EQ_INT(r, 1, "嵌套括号成功");
        ASSERT_EQ_INT(*open, '(', "open 指向外层 (");
        ASSERT_EQ_INT(*close, ')', "close 指向外层 )");
        ENDCASE;
    }

    CASE("find_parens: 注释中的括号不计");
    {
        const char *open, *close;
        int r = analyze_find_parens("f /* ( */ () {", &open, &close);
        ASSERT_EQ_INT(r, 1, "跳过注释成功");
        ENDCASE;
    }

    CASE("find_parens: 无括号有 {");
    {
        const char *open, *close;
        int r = analyze_find_parens("x {", &open, &close);
        ASSERT_EQ_INT(r, 0, "无括号返回 0");
        ENDCASE;
    }

    CASE("find_parens: 字符串中的括号不计");
    {
        const char *open, *close;
        int r = analyze_find_parens("f \"(\" () {", &open, &close);
        ASSERT_EQ_INT(r, 1, "跳过字符串成功");
        ENDCASE;
    }
}

/* ================================================================== */
/* 10. analyze_extract_func_name                                      */
/* ================================================================== */
TEST_SUITE(test_extract_func_name) {
    CASE("extract_func_name: int main()");
    {
        const char *start = "int main()";
        const char *open = strchr(start, '(');
        const char *ns, *ne;
        int r = analyze_extract_func_name(start, open, &ns, &ne);
        ASSERT_EQ_INT(r, 1, "应成功");
        ASSERT_EQ_INT(ne - ns, 4, "name 长度 4");
        ASSERT_TRUE(strncmp(ns, "main", 4) == 0, "name=main");
        ENDCASE;
    }

    CASE("extract_func_name: 无返回类型");
    {
        const char *start = "main()";
        const char *open = strchr(start, '(');
        const char *ns, *ne;
        int r = analyze_extract_func_name(start, open, &ns, &ne);
        ASSERT_EQ_INT(r, 0, "无类型返回 0");
        ENDCASE;
    }

    CASE("extract_func_name: void *func()");
    {
        const char *start = "void *func()";
        const char *open = strchr(start, '(');
        const char *ns, *ne;
        int r = analyze_extract_func_name(start, open, &ns, &ne);
        ASSERT_EQ_INT(r, 1, "应成功");
        ASSERT_TRUE(strncmp(ns, "func", 4) == 0, "name=func");
        ENDCASE;
    }

    CASE("extract_func_name: 带空格的返回类型");
    {
        const char *start = "unsigned  int  myfunc()";
        const char *open = strchr(start, '(');
        const char *ns, *ne;
        int r = analyze_extract_func_name(start, open, &ns, &ne);
        ASSERT_EQ_INT(r, 1, "应成功");
        ASSERT_TRUE(strncmp(ns, "myfunc", 6) == 0, "name=myfunc");
        ENDCASE;
    }
}

/* ================================================================== */
/* 11. analyze_try_extract_function / analyze_extract_functions       */
/* ================================================================== */
TEST_SUITE(test_extract_functions) {
    CASE("try_extract: 正常函数");
    {
        AnalyzeFunc funcs[1];
        int count = 0;
        const char *next = NULL;
        int r = analyze_try_extract_function("int main() { return 0; }", "mod",
                                             funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 1, "应成功");
        ASSERT_EQ_INT(count, 1, "count=1");
        ASSERT_EQ_STR(funcs[0].name, "main", "name=main");
        ASSERT_EQ_STR(funcs[0].module, "mod", "module=mod");
        ASSERT_NOT_NULL(funcs[0].code, "code 非空");
        ASSERT_NOT_NULL(next, "next 非空");
        free(funcs[0].code);
        ENDCASE;
    }

    CASE("try_extract: 关键字名拒绝");
    {
        AnalyzeFunc funcs[1];
        int count = 0;
        const char *next = NULL;
        int r = analyze_try_extract_function("if (x) { return 0; }", "mod",
                                             funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 0, "关键字拒绝");
        ASSERT_EQ_INT(count, 0, "count=0");
        ENDCASE;
    }

    CASE("try_extract: 无大括号拒绝");
    {
        AnalyzeFunc funcs[1];
        int count = 0;
        const char *next = NULL;
        int r = analyze_try_extract_function("int main();", "mod",
                                             funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 0, "无大括号拒绝");
        ENDCASE;
    }

    CASE("try_extract: 无返回类型拒绝");
    {
        AnalyzeFunc funcs[1];
        int count = 0;
        const char *next = NULL;
        int r = analyze_try_extract_function("main() { return 0; }", "mod",
                                             funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 0, "无返回类型拒绝");
        ENDCASE;
    }

    CASE("try_extract: 容量满拒绝");
    {
        AnalyzeFunc funcs[1];
        int count = 1; /* 已满 */
        const char *next = NULL;
        int r = analyze_try_extract_function("int main() { return 0; }", "mod",
                                             funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 0, "容量满拒绝");
        ENDCASE;
    }

    CASE("try_extract: 空串拒绝");
    {
        AnalyzeFunc funcs[1];
        int count = 0;
        const char *next = NULL;
        int r = analyze_try_extract_function("", "mod", funcs, &count, 1, &next);
        ASSERT_EQ_INT(r, 0, "空串拒绝");
        ENDCASE;
    }

    CASE("extract_functions: NULL/空代码");
    {
        AnalyzeFunc funcs[10];
        int count = 0;
        analyze_extract_functions("mod", NULL, funcs, &count, 10);
        ASSERT_EQ_INT(count, 0, "NULL code");
        analyze_extract_functions("mod", "", funcs, &count, 10);
        ASSERT_EQ_INT(count, 0, "空 code");
        ENDCASE;
    }

    CASE("extract_functions: 多个函数");
    {
        const char *code =
            "int add(int a, int b) {\n    return a + b;\n}\n"
            "int sub(int a, int b) {\n    return a - b;\n}\n";
        AnalyzeFunc funcs[10];
        int count = 0;
        analyze_extract_functions("mod", code, funcs, &count, 10);
        ASSERT_EQ_INT(count, 2, "应提取 2 个函数");
        ASSERT_EQ_STR(funcs[0].name, "add", "第一个 add");
        ASSERT_EQ_STR(funcs[1].name, "sub", "第二个 sub");
        for (int i = 0; i < count; i++) free(funcs[i].code);
        ENDCASE;
    }

    CASE("extract_functions: 跳过注释中的函数");
    {
        const char *code =
            "/* int fake() { return 0; } */\n"
            "int real() { return 1; }\n";
        AnalyzeFunc funcs[10];
        int count = 0;
        analyze_extract_functions("mod", code, funcs, &count, 10);
        ASSERT_EQ_INT(count, 1, "只提取 1 个（跳过注释中的）");
        ASSERT_EQ_STR(funcs[0].name, "real", "name=real");
        for (int i = 0; i < count; i++) free(funcs[i].code);
        ENDCASE;
    }

    CASE("extract_functions: 跳过预处理行");
    {
        const char *code =
            "#define MAX 100\n"
            "int bar() { return MAX; }\n";
        AnalyzeFunc funcs[10];
        int count = 0;
        analyze_extract_functions("mod", code, funcs, &count, 10);
        ASSERT_EQ_INT(count, 1, "提取 1 个函数");
        ASSERT_EQ_STR(funcs[0].name, "bar", "name=bar");
        for (int i = 0; i < count; i++) free(funcs[i].code);
        ENDCASE;
    }

    CASE("extract_functions: 带参数的函数");
    {
        const char *code =
            "void process(int x, char *s) {\n    x++;\n}\n";
        AnalyzeFunc funcs[10];
        int count = 0;
        analyze_extract_functions("mod", code, funcs, &count, 10);
        ASSERT_EQ_INT(count, 1, "提取 1 个");
        ASSERT_EQ_STR(funcs[0].name, "process", "name=process");
        for (int i = 0; i < count; i++) free(funcs[i].code);
        ENDCASE;
    }
}

/* ================================================================== */
/* 12. analyze_is_two_char_op                                         */
/* ================================================================== */
TEST_SUITE(test_is_two_char_op) {
    CASE("is_two_char_op: 各运算符");
    ASSERT_EQ_INT(analyze_is_two_char_op('=', '='), 1, "==");
    ASSERT_EQ_INT(analyze_is_two_char_op('!', '='), 1, "!=");
    ASSERT_EQ_INT(analyze_is_two_char_op('<', '='), 1, "<=");
    ASSERT_EQ_INT(analyze_is_two_char_op('>', '='), 1, ">=");
    ASSERT_EQ_INT(analyze_is_two_char_op('&', '&'), 1, "&&");
    ASSERT_EQ_INT(analyze_is_two_char_op('|', '|'), 1, "||");
    ASSERT_EQ_INT(analyze_is_two_char_op('+', '+'), 1, "++");
    ASSERT_EQ_INT(analyze_is_two_char_op('-', '-'), 1, "--");
    ASSERT_EQ_INT(analyze_is_two_char_op('-', '>'), 1, "->");
    ASSERT_EQ_INT(analyze_is_two_char_op('<', '<'), 1, "<<");
    ASSERT_EQ_INT(analyze_is_two_char_op('>', '>'), 1, ">>");
    ASSERT_EQ_INT(analyze_is_two_char_op('+', '='), 1, "+=");
    ASSERT_EQ_INT(analyze_is_two_char_op('-', '='), 1, "-=");
    ASSERT_EQ_INT(analyze_is_two_char_op('*', '='), 1, "*=");
    ASSERT_EQ_INT(analyze_is_two_char_op('/', '='), 1, "/=");
    ASSERT_EQ_INT(analyze_is_two_char_op('%', '='), 1, "%=");
    ASSERT_EQ_INT(analyze_is_two_char_op('&', '='), 1, "&=");
    ASSERT_EQ_INT(analyze_is_two_char_op('|', '='), 1, "|=");
    ASSERT_EQ_INT(analyze_is_two_char_op('^', '='), 1, "^=");
    ASSERT_EQ_INT(analyze_is_two_char_op('.', '.'), 1, "..");
    ENDCASE;

    CASE("is_two_char_op: 非运算符");
    ASSERT_EQ_INT(analyze_is_two_char_op('a', 'b'), 0, "ab");
    ASSERT_EQ_INT(analyze_is_two_char_op('(', ')'), 0, "()");
    ASSERT_EQ_INT(analyze_is_two_char_op('+', '-'), 0, "+-");
    ENDCASE;
}

/* ================================================================== */
/* 13. analyze_emit_literal / analyze_emit_identifier /               */
/*     analyze_emit_operator / analyze_emit_literal_token             */
/* ================================================================== */
TEST_SUITE(test_emit_functions) {
    CASE("emit_literal: 正常添加");
    {
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal(toks, &count, 5, "NUM");
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_INT(count, 1, "count=1");
        ASSERT_EQ_STR(toks[0].text, "NUM", "text=NUM");
        ENDCASE;
    }

    CASE("emit_literal: 容量满返回 0");
    {
        AnalyzeToken toks[1];
        int count = 1;
        int r = analyze_emit_literal(toks, &count, 1, "NUM");
        ASSERT_EQ_INT(r, 0, "容量满返回 0");
        ASSERT_EQ_INT(count, 1, "count 不变");
        ENDCASE;
    }

    CASE("emit_identifier: 正常标识符");
    {
        const char *p = "hello123 world";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_identifier(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_INT(count, 1, "count=1");
        ASSERT_EQ_STR(toks[0].text, "hello123", "text=hello123");
        ASSERT_EQ_INT(*p, ' ', "p 指向空格");
        ENDCASE;
    }

    CASE("emit_identifier: 下划线开头");
    {
        const char *p = "_var";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_identifier(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "_var", "text=_var");
        ENDCASE;
    }

    CASE("emit_identifier: 非标识符开头返回 0");
    {
        const char *p = "123";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_identifier(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 0, "数字开头返回 0");
        ENDCASE;
    }

    CASE("emit_identifier: 容量满返回 0");
    {
        const char *p = "hello";
        AnalyzeToken toks[1];
        int count = 1;
        int r = analyze_emit_identifier(&p, toks, &count, 1);
        ASSERT_EQ_INT(r, 0, "容量满返回 0");
        ENDCASE;
    }

    CASE("emit_identifier: 长标识符截断到 63 字符");
    {
        /* 70 字符的标识符 */
        const char *p = "a1234567890123456789012345678901234567890123456789012345678901234567890";
        AnalyzeToken toks[1];
        int count = 0;
        analyze_emit_identifier(&p, toks, &count, 1);
        ASSERT_EQ_INT((int)strlen(toks[0].text), 63, "截断到 63 字符");
        ASSERT_EQ_INT(*p, '\0', "p 前进到末尾");
        ENDCASE;
    }

    CASE("emit_operator: 双字符运算符");
    {
        const char *p = "== x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_operator(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "==", "text====");
        ASSERT_EQ_INT(*p, ' ', "p 前进 2");
        ENDCASE;
    }

    CASE("emit_operator: 单字符运算符");
    {
        const char *p = "; x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_operator(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, ";", "text=;");
        ASSERT_EQ_INT(*p, ' ', "p 前进 1");
        ENDCASE;
    }

    CASE("emit_operator: 非标点返回 0");
    {
        const char *p = "x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_operator(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 0, "非标点返回 0");
        ENDCASE;
    }

    CASE("emit_operator: 容量满返回 0");
    {
        const char *p = "+";
        AnalyzeToken toks[1];
        int count = 1;
        int r = analyze_emit_operator(&p, toks, &count, 1);
        ASSERT_EQ_INT(r, 0, "容量满返回 0");
        ENDCASE;
    }

    CASE("emit_literal_token: 字符串");
    {
        const char *p = "\"hello\" x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "\"STR\"", "text=\"STR\"");
        ASSERT_EQ_INT(*p, ' ', "p 跳过字符串");
        ENDCASE;
    }

    CASE("emit_literal_token: 字符");
    {
        const char *p = "'c' x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "'C'", "text='C'");
        ENDCASE;
    }

    CASE("emit_literal_token: 数字");
    {
        const char *p = "42 x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "NUM", "text=NUM");
        ASSERT_EQ_INT(*p, ' ', "p 跳过数字");
        ENDCASE;
    }

    CASE("emit_literal_token: 十六进制数字");
    {
        const char *p = "0x1F x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "NUM", "text=NUM");
        ENDCASE;
    }

    CASE("emit_literal_token: 浮点数");
    {
        const char *p = ".5 x";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_STR(toks[0].text, "NUM", "text=NUM");
        ENDCASE;
    }

    CASE("emit_literal_token: 非字面量返回 0");
    {
        const char *p = "hello";
        AnalyzeToken toks[5];
        int count = 0;
        int r = analyze_emit_literal_token(&p, toks, &count, 5);
        ASSERT_EQ_INT(r, 0, "标识符返回 0");
        ENDCASE;
    }
}

/* ================================================================== */
/* 14. analyze_skip_trivia                                            */
/* ================================================================== */
TEST_SUITE(test_skip_trivia) {
    CASE("skip_trivia: 空格");
    {
        const char *p = " x";
        int r = analyze_skip_trivia(&p);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_INT(*p, 'x', "p 前进");
        ENDCASE;
    }

    CASE("skip_trivia: 行注释");
    {
        const char *p = "// c\nx";
        int r = analyze_skip_trivia(&p);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_INT(*p, '\n', "p 到换行");
        ENDCASE;
    }

    CASE("skip_trivia: 块注释");
    {
        const char *p = "/* c */x";
        int r = analyze_skip_trivia(&p);
        ASSERT_EQ_INT(r, 1, "返回 1");
        ASSERT_EQ_INT(*p, 'x', "p 到注释后");
        ENDCASE;
    }

    CASE("skip_trivia: 代码字符返回 0");
    {
        const char *p = "int";
        int r = analyze_skip_trivia(&p);
        ASSERT_EQ_INT(r, 0, "返回 0");
        ASSERT_EQ_INT(*p, 'i', "p 不变");
        ENDCASE;
    }

    CASE("skip_trivia: 连续跳过");
    {
        const char *p = "  /* c */ // d\nx";
        int steps = 0;
        while (analyze_skip_trivia(&p)) steps++;
        ASSERT_TRUE(steps >= 3, "至少跳过 3 次");
        ASSERT_EQ_INT(*p, 'x', "p 到 x");
        ENDCASE;
    }
}

/* ================================================================== */
/* 15. analyze_tokenize                                               */
/* ================================================================== */
TEST_SUITE(test_tokenize) {
    CASE("tokenize: 简单表达式");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("int x = 42;", toks, 20);
        ASSERT_EQ_INT(n, 5, "5 个 token");
        ASSERT_EQ_STR(toks[0].text, "int", "tok0=int");
        ASSERT_EQ_STR(toks[1].text, "x", "tok1=x");
        ASSERT_EQ_STR(toks[2].text, "=", "tok2==");
        ASSERT_EQ_STR(toks[3].text, "NUM", "tok3=NUM");
        ASSERT_EQ_STR(toks[4].text, ";", "tok4=;");
        ENDCASE;
    }

    CASE("tokenize: 带注释");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("int /* c */ x;", toks, 20);
        ASSERT_EQ_INT(n, 3, "3 个 token（跳过注释）");
        ASSERT_EQ_STR(toks[0].text, "int", "tok0=int");
        ASSERT_EQ_STR(toks[1].text, "x", "tok1=x");
        ASSERT_EQ_STR(toks[2].text, ";", "tok2=;");
        ENDCASE;
    }

    CASE("tokenize: 带字符串");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("char *s = \"hello\";", toks, 20);
        ASSERT_EQ_INT(n, 6, "6 个 token");
        ASSERT_EQ_STR(toks[4].text, "\"STR\"", "字符串归一化");
        ENDCASE;
    }

    CASE("tokenize: 双字符运算符");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("a == b;", toks, 20);
        ASSERT_EQ_INT(n, 4, "4 个 token");
        ASSERT_EQ_STR(toks[1].text, "==", "tok1====");
        ENDCASE;
    }

    CASE("tokenize: 空串");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("", toks, 20);
        ASSERT_EQ_INT(n, 0, "0 个 token");
        ENDCASE;
    }

    CASE("tokenize: 容量限制");
    {
        AnalyzeToken toks[3];
        int n = analyze_tokenize("a b c d e f", toks, 3);
        ASSERT_EQ_INT(n, 3, "最多 3 个 token");
        ENDCASE;
    }

    CASE("tokenize: if 语句带 &&");
    {
        AnalyzeToken toks[20];
        int n = analyze_tokenize("if (x && y) {", toks, 20);
        ASSERT_EQ_INT(n, 7, "7 个 token");
        ASSERT_EQ_STR(toks[0].text, "if", "tok0=if");
        ASSERT_EQ_STR(toks[3].text, "&&", "tok3=&&");
        ENDCASE;
    }
}

/* ================================================================== */
/* 16. analyze_cyclomatic_complexity                                  */
/* ================================================================== */
TEST_SUITE(test_cyclomatic_complexity) {
    CASE("complexity: NULL 返回 1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity(NULL), 1, "NULL→1");
    ENDCASE;

    CASE("complexity: 空串返回 1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity(""), 1, "空→1");
    ENDCASE;

    CASE("complexity: 无分支返回 1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("x = 1;"), 1, "无分支→1");
    ENDCASE;

    CASE("complexity: if +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("if (x) y;"), 2, "if→2");
    ENDCASE;

    CASE("complexity: else +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("if (x) y; else z;"), 3, "if+else→3");
    ENDCASE;

    CASE("complexity: for +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("for (i=0;i<n;i++) {}"), 2, "for→2");
    ENDCASE;

    CASE("complexity: while +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("while (x) {}"), 2, "while→2");
    ENDCASE;

    CASE("complexity: case +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("case 1: break;"), 2, "case→2");
    ENDCASE;

    CASE("complexity: && +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("x && y;"), 2, "&&→2");
    ENDCASE;

    CASE("complexity: || +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("x || y;"), 2, "||→2");
    ENDCASE;

    CASE("complexity: ? 三目 +1");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("x ? y : z;"), 2, "?→2");
    ENDCASE;

    CASE("complexity: 组合 if && ||");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("if (x && y || z) {}"), 4, "if+&&+||→4");
    ENDCASE;

    CASE("complexity: 多个 if");
    ASSERT_EQ_INT(analyze_cyclomatic_complexity("if (a) {} if (b) {} if (c) {}"), 4, "3个if→4");
    ENDCASE;
}

/* ================================================================== */
/* 17. analyze_ngram_equal                                            */
/* ================================================================== */
TEST_SUITE(test_ngram_equal) {
    CASE("ngram_equal: 相同");
    {
        AnalyzeToken a[8], b[8];
        fill_gram(a, "a", "b", "c", "d", "e", "f", "g", "h");
        fill_gram(b, "a", "b", "c", "d", "e", "f", "g", "h");
        ASSERT_EQ_INT(analyze_ngram_equal(a, b), 1, "应相等");
        ENDCASE;
    }

    CASE("ngram_equal: 不同");
    {
        AnalyzeToken a[8], b[8];
        fill_gram(a, "a", "b", "c", "d", "e", "f", "g", "h");
        fill_gram(b, "a", "b", "c", "d", "e", "f", "g", "X");
        ASSERT_EQ_INT(analyze_ngram_equal(a, b), 0, "应不等");
        ENDCASE;
    }

    CASE("ngram_equal: 首token不同");
    {
        AnalyzeToken a[8], b[8];
        fill_gram(a, "a", "b", "c", "d", "e", "f", "g", "h");
        fill_gram(b, "X", "b", "c", "d", "e", "f", "g", "h");
        ASSERT_EQ_INT(analyze_ngram_equal(a, b), 0, "应不等");
        ENDCASE;
    }
}

/* ================================================================== */
/* 18. analyze_ngram_match_pattern / analyze_ngram_is_common          */
/* ================================================================== */
TEST_SUITE(test_ngram_patterns) {
    CASE("match_pattern: 完全匹配");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "for", "(", "int", "i", "=", "NUM", ";", "i");
        AnalyzeNGramPattern pat = { {"for","(","int","i","=","NUM",";","i"} };
        ASSERT_EQ_INT(analyze_ngram_match_pattern(gram, &pat), 1, "应匹配");
        ENDCASE;
    }

    CASE("match_pattern: 不匹配");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "foo", "bar", "baz", "qux", "a", "b", "c", "d");
        AnalyzeNGramPattern pat = { {"for","(","int","i","=","NUM",";","i"} };
        ASSERT_EQ_INT(analyze_ngram_match_pattern(gram, &pat), 0, "应不匹配");
        ENDCASE;
    }

    CASE("match_pattern: 通配符匹配");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "if", "(", "!", "ptr", ")", "return", "0", ";");
        AnalyzeNGramPattern pat = { {"if","(","!",NULL,")","return",NULL,NULL} };
        ASSERT_EQ_INT(analyze_ngram_match_pattern(gram, &pat), 1, "通配符应匹配");
        ENDCASE;
    }

    CASE("match_pattern: 通配符位置不匹配");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "if", "(", "x", "ptr", ")", "return", "0", ";");
        AnalyzeNGramPattern pat = { {"if","(","!",NULL,")","return",NULL,NULL} };
        ASSERT_EQ_INT(analyze_ngram_match_pattern(gram, &pat), 0, "第3位 ! 不匹配");
        ENDCASE;
    }

    CASE("is_common: for 循环模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "for", "(", "int", "i", "=", "NUM", ";", "i");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "for 循环模板常见");
        ENDCASE;
    }

    CASE("is_common: if (!x) return 模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "if", "(", "!", "ptr", ")", "return", "-", "NUM");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "if(!x)return 常见");
        ENDCASE;
    }

    CASE("is_common: free 模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "free", "(", "ptr", ")", ";", "ptr", "=", "NULL");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "free 常见");
        ENDCASE;
    }

    CASE("is_common: = NULL 模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "ptr", "=", "NULL", ";", "x", "y", "z", "w");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "= NULL 常见");
        ENDCASE;
    }

    CASE("is_common: 非常见 n-gram");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "compute", "(", "value", ")", ";", "return", "result", ";");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 0, "非常见");
        ENDCASE;
    }

    CASE("is_common: for j 循环模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "for", "(", "int", "j", "=", "NUM", ";", "j");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "for j 模板常见");
        ENDCASE;
    }

    CASE("is_common: for k 循环模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "for", "(", "int", "k", "=", "NUM", ";", "k");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "for k 模板常见");
        ENDCASE;
    }

    CASE("is_common: return -NUM 模板");
    {
        AnalyzeToken gram[8];
        fill_gram(gram, "x", ";", "return", "-", "NUM", ";", "y", "z");
        ASSERT_EQ_INT(analyze_ngram_is_common(gram), 1, "return -NUM 常见");
        ENDCASE;
    }
}

/* ================================================================== */
/* 19. analyze_build_module_path                                      */
/* ================================================================== */
TEST_SUITE(test_build_module_path) {
    CASE("build_module_path: root 返回空串");
    {
        reset_proj();
        char buf[256];
        analyze_build_module_path(g_proj->root, buf, sizeof(buf));
        ASSERT_EQ_STR(buf, "", "root 路径为空");
        ENDCASE;
    }

    CASE("build_module_path: 单层模块");
    {
        reset_proj();
        ModuleDomain *m = domain_module_domain_new("mymod");
        domain_domain_add_child(g_proj->root, (Domain *)m);
        char buf[256];
        analyze_build_module_path((Domain *)m, buf, sizeof(buf));
        ASSERT_EQ_STR(buf, "mymod", "路径=mymod");
        ENDCASE;
    }

    CASE("build_module_path: 嵌套模块");
    {
        reset_proj();
        ModuleDomain *parent = domain_module_domain_new("parent");
        domain_domain_add_child(g_proj->root, (Domain *)parent);
        ModuleDomain *child = domain_module_domain_new("child");
        domain_domain_add_child((Domain *)parent, (Domain *)child);
        char buf[256];
        analyze_build_module_path((Domain *)child, buf, sizeof(buf));
        ASSERT_EQ_STR(buf, "parent/child", "路径=parent/child");
        ENDCASE;
    }

    CASE("build_module_path: 三层嵌套");
    {
        reset_proj();
        ModuleDomain *a = domain_module_domain_new("a");
        domain_domain_add_child(g_proj->root, (Domain *)a);
        ModuleDomain *b = domain_module_domain_new("b");
        domain_domain_add_child((Domain *)a, (Domain *)b);
        ModuleDomain *c = domain_module_domain_new("c");
        domain_domain_add_child((Domain *)b, (Domain *)c);
        char buf[256];
        analyze_build_module_path((Domain *)c, buf, sizeof(buf));
        ASSERT_EQ_STR(buf, "a/b/c", "路径=a/b/c");
        ENDCASE;
    }
}

/* ================================================================== */
/* 20. analyze_collect_modules                                        */
/* ================================================================== */
TEST_SUITE(test_collect_modules) {
    CASE("collect_modules: NULL 安全");
    {
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(NULL, mods, &count, 5);
        ASSERT_EQ_INT(count, 0, "NULL 不崩溃");
        ENDCASE;
    }

    CASE("collect_modules: 空项目");
    {
        reset_proj();
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        /* root 无 code，不被收集 */
        ASSERT_EQ_INT(count, 0, "无模块收集");
        ENDCASE;
    }

    CASE("collect_modules: SRC 模块有 code 被收集");
    {
        reset_proj();
        add_mod("mod1", "int x;\n");
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        ASSERT_EQ_INT(count, 1, "收集 1 个模块");
        ASSERT_EQ_STR(mods[0].name, "mod1", "name=mod1");
        ASSERT_NOT_NULL(mods[0].code, "code 非空");
        ENDCASE;
    }

    CASE("collect_modules: SRC 模块无 code 不收集");
    {
        reset_proj();
        ModuleDomain *m = domain_module_domain_new("nomod");
        domain_domain_add_child(g_proj->root, (Domain *)m);
        /* 不设 code */
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        ASSERT_EQ_INT(count, 0, "无 code 不收集");
        ENDCASE;
    }

    CASE("collect_modules: 非 SRC 模块不收集");
    {
        reset_proj();
        ModuleDomain *m = domain_module_domain_new("ext");
        m->mode = MOD_MODE_EXTERNAL;
        domain_domain_set_code((Domain *)m, "int x;\n");
        domain_domain_add_child(g_proj->root, (Domain *)m);
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        ASSERT_EQ_INT(count, 0, "EXTERNAL 不收集");
        ENDCASE;
    }

    CASE("collect_modules: 多模块递归");
    {
        reset_proj();
        add_mod("mod1", "int x;\n");
        add_mod("mod2", "int y;\n");
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        ASSERT_EQ_INT(count, 2, "收集 2 个模块");
        ENDCASE;
    }

    CASE("collect_modules: 嵌套模块递归");
    {
        reset_proj();
        ModuleDomain *parent = domain_module_domain_new("parent");
        domain_domain_set_code((Domain *)parent, "int p;\n");
        domain_domain_add_child(g_proj->root, (Domain *)parent);
        ModuleDomain *child = domain_module_domain_new("child");
        domain_domain_set_code((Domain *)child, "int c;\n");
        domain_domain_add_child((Domain *)parent, (Domain *)child);
        AnalyzeMod mods[5];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 5);
        ASSERT_EQ_INT(count, 2, "收集 2 个（含嵌套）");
        ASSERT_EQ_STR(mods[0].name, "parent", "第一个=parent");
        ASSERT_EQ_STR(mods[1].name, "parent/child", "第二个=parent/child");
        ENDCASE;
    }

    CASE("collect_modules: 容量限制");
    {
        reset_proj();
        add_mod("m1", "x;\n");
        add_mod("m2", "x;\n");
        add_mod("m3", "x;\n");
        AnalyzeMod mods[2];
        int count = 0;
        analyze_collect_modules(g_proj->root, mods, &count, 2);
        ASSERT_EQ_INT(count, 2, "容量限制 2");
        ENDCASE;
    }
}

/* ================================================================== */
/* 21. analyze_has_call                                               */
/* ================================================================== */
TEST_SUITE(test_has_call) {
    CASE("has_call: NULL 输入返回 0");
    ASSERT_EQ_INT(analyze_has_call(NULL, "f"), 0, "NULL code");
    ASSERT_EQ_INT(analyze_has_call("f()", NULL), 0, "NULL name");
    ASSERT_EQ_INT(analyze_has_call("f()", ""), 0, "空 name");
    ASSERT_EQ_INT(analyze_has_call("", "f"), 0, "空 code");
    ENDCASE;

    CASE("has_call: 存在调用");
    ASSERT_EQ_INT(analyze_has_call("f();", "f"), 1, "f() 存在");
    ENDCASE;

    CASE("has_call: 名字后有空格再 (");
    ASSERT_EQ_INT(analyze_has_call("f (x);", "f"), 1, "f (x) 存在");
    ENDCASE;

    CASE("has_call: 不存在调用");
    ASSERT_EQ_INT(analyze_has_call("g();", "f"), 0, "g() 不是 f");
    ENDCASE;

    CASE("has_call: 子串匹配不算（前缀）");
    /* "foobar" 不是 "foo" 的完整调用 */
    ASSERT_EQ_INT(analyze_has_call("foobar();", "foo"), 0, "foobar 不是 foo");
    ENDCASE;

    CASE("has_call: 注释中的调用不算");
    ASSERT_EQ_INT(analyze_has_call("/* f() */ g();", "f"), 0, "注释中的 f 不算");
    ENDCASE;

    CASE("has_call: 字符串中的调用不算");
    ASSERT_EQ_INT(analyze_has_call("\"f()\"; g();", "f"), 0, "字符串中的 f 不算");
    ENDCASE;

    CASE("has_call: 预处理行中的不算");
    ASSERT_EQ_INT(analyze_has_call("#f()\nx;", "f"), 0, "预处理行中的 f 不算");
    ENDCASE;

    CASE("has_call: 多次调用");
    ASSERT_EQ_INT(analyze_has_call("f(); f(); f();", "f"), 1, "多次调用仍返回 1");
    ENDCASE;

    CASE("has_call: 调用在表达式中");
    ASSERT_EQ_INT(analyze_has_call("x = f(a, b) + g();", "f"), 1, "f 在表达式中");
    ASSERT_EQ_INT(analyze_has_call("x = f(a, b) + g();", "g"), 1, "g 在表达式中");
    ENDCASE;
}

/* ================================================================== */
/* 22. analyze_resolve_call                                           */
/* ================================================================== */
TEST_SUITE(test_resolve_call) {
    CASE("resolve_call: 同模块优先");
    {
        AnalyzeFunc funcs[3] = {
            {"f", "modA", NULL, 0, 0},
            {"f", "modB", NULL, 0, 0},
            {"caller", "modA", NULL, 0, 0},
        };
        int idx = analyze_resolve_call("f", "modA", funcs, 3);
        ASSERT_EQ_INT(idx, 0, "同模块 modA 优先 → 索引 0");
        ENDCASE;
    }

    CASE("resolve_call: 全局唯一兜底");
    {
        AnalyzeFunc funcs[2] = {
            {"helper", "modA", NULL, 0, 0},
            {"caller", "modB", NULL, 0, 0},
        };
        int idx = analyze_resolve_call("helper", "modB", funcs, 2);
        ASSERT_EQ_INT(idx, 0, "全局唯一 → 索引 0");
        ENDCASE;
    }

    CASE("resolve_call: 多个同名跨模块无法定位");
    {
        AnalyzeFunc funcs[3] = {
            {"f", "modA", NULL, 0, 0},
            {"f", "modB", NULL, 0, 0},
            {"caller", "modC", NULL, 0, 0},
        };
        int idx = analyze_resolve_call("f", "modC", funcs, 3);
        ASSERT_EQ_INT(idx, -1, "多个同名跨模块 → -1");
        ENDCASE;
    }

    CASE("resolve_call: 不存在返回 -1");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "modA", NULL, 0, 0},
            {"g", "modB", NULL, 0, 0},
        };
        int idx = analyze_resolve_call("h", "modA", funcs, 2);
        ASSERT_EQ_INT(idx, -1, "不存在 → -1");
        ENDCASE;
    }

    CASE("resolve_call: 空列表返回 -1");
    {
        int idx = analyze_resolve_call("f", "modA", NULL, 0);
        ASSERT_EQ_INT(idx, -1, "空列表 → -1");
        ENDCASE;
    }
}

/* ================================================================== */
/* 23. dependency_compute_counts / dependency_report /                */
/*     commands_analyze_dependency_complexity                          */
/* ================================================================== */
TEST_SUITE(test_dependency) {
    CASE("compute_counts: 单个被调用");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "m", "return 0;", 0, 0},
            {"g", "m", "f();", 0, 0},
        };
        dependency_compute_counts(funcs, 2);
        ASSERT_EQ_INT(funcs[0].call_count, 1, "f 被 g 调用 1 次");
        ASSERT_EQ_INT(funcs[1].call_count, 0, "g 未被调用");
        ENDCASE;
    }

    CASE("compute_counts: 多个调用方");
    {
        AnalyzeFunc funcs[3] = {
            {"f", "m", "return 0;", 0, 0},
            {"g", "m", "f();", 0, 0},
            {"h", "m", "f();", 0, 0},
        };
        dependency_compute_counts(funcs, 3);
        ASSERT_EQ_INT(funcs[0].call_count, 2, "f 被 g 和 h 调用");
        ASSERT_EQ_INT(funcs[1].call_count, 0, "g 未被调用");
        ASSERT_EQ_INT(funcs[2].call_count, 0, "h 未被调用");
        ENDCASE;
    }

    CASE("compute_counts: 无调用");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "m", "return 0;", 0, 0},
            {"g", "m", "return 1;", 0, 0},
        };
        dependency_compute_counts(funcs, 2);
        ASSERT_EQ_INT(funcs[0].call_count, 0, "f 未被调用");
        ASSERT_EQ_INT(funcs[1].call_count, 0, "g 未被调用");
        ENDCASE;
    }

    CASE("compute_counts: 同模块优先解析");
    {
        AnalyzeFunc funcs[3] = {
            {"f", "modA", "return 0;", 0, 0},
            {"f", "modB", "return 0;", 0, 0},
            {"caller", "modA", "f();", 0, 0},
        };
        dependency_compute_counts(funcs, 3);
        /* caller 在 modA, 调用 f, 同模块优先 → 解析为 funcs[0] */
        ASSERT_EQ_INT(funcs[0].call_count, 1, "modA 的 f 被调用");
        ASSERT_EQ_INT(funcs[1].call_count, 0, "modB 的 f 未被调用");
        ENDCASE;
    }

    CASE("compute_counts: 注释中的调用不算");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "m", "return 0;", 0, 0},
            {"g", "m", "/* f(); */ return 1;", 0, 0},
        };
        dependency_compute_counts(funcs, 2);
        ASSERT_EQ_INT(funcs[0].call_count, 0, "注释中的调用不算");
        ENDCASE;
    }

    CASE("dependency_report: 正常打印不崩溃");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "m", "return 0;", 0, 2},
            {"g", "m", "f();", 0, 0},
        };
        suppress_stdout();
        dependency_report(funcs, 2);
        restore_stdout();
        ASSERT_TRUE(1, "正常打印不崩溃");
        ENDCASE;
    }

    CASE("dependency_report: 空函数列表");
    {
        suppress_stdout();
        dependency_report(NULL, 0);
        restore_stdout();
        ASSERT_TRUE(1, "空列表不崩溃");
        ENDCASE;
    }

    CASE("commands_analyze_dependency_complexity: 集成");
    {
        AnalyzeFunc funcs[2] = {
            {"f", "m", "return 0;", 0, 0},
            {"g", "m", "f();", 0, 0},
        };
        suppress_stdout();
        commands_analyze_dependency_complexity(funcs, 2);
        restore_stdout();
        ASSERT_EQ_INT(funcs[0].call_count, 1, "f call_count=1");
        ENDCASE;
    }
}

/* ================================================================== */
/* 24. analyze_ngram_is_dup / analyze_print_first_dup_pair /          */
/*     analyze_print_dup_fragments / commands_analyze_duplication      */
/* ================================================================== */
TEST_SUITE(test_ngram_dup) {
    CASE("ngram_is_dup: 存在重复");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("a b c d e f g h", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("a b c d e f g h", toks1, ANALYZE_MAX_TOKENS);
        /* n-gram 0 of func 0 应在 func 1 中找到 */
        int is_dup = analyze_ngram_is_dup(all_toks, tok_counts, 2, 0, 0);
        ASSERT_EQ_INT(is_dup, 1, "应检测到重复");
        ENDCASE;
    }

    CASE("ngram_is_dup: 无重复");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("a b c d e f g h", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("x y z w p q r s", toks1, ANALYZE_MAX_TOKENS);
        int is_dup = analyze_ngram_is_dup(all_toks, tok_counts, 2, 0, 0);
        ASSERT_EQ_INT(is_dup, 0, "应无重复");
        ENDCASE;
    }

    CASE("ngram_is_dup: token 不足跳过");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        /* func 1 只有 3 个 token，不足 8 个，无法形成 n-gram */
        tok_counts[0] = analyze_tokenize("a b c d e f g h", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("x y z", toks1, ANALYZE_MAX_TOKENS);
        int is_dup = analyze_ngram_is_dup(all_toks, tok_counts, 2, 0, 0);
        ASSERT_EQ_INT(is_dup, 0, "func 1 token 不足 → 无重复");
        ENDCASE;
    }

    CASE("print_first_dup_pair: 找到重复");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("a b c d e f g h", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("a b c d e f g h", toks1, ANALYZE_MAX_TOKENS);
        AnalyzeFunc funcs[2] = {
            {"fa", "ma", NULL, 0, 0},
            {"fb", "mb", NULL, 0, 0},
        };
        suppress_stdout();
        int r = analyze_print_first_dup_pair(all_toks, tok_counts, 0, 1, funcs);
        restore_stdout();
        ASSERT_EQ_INT(r, 1, "应找到并打印");
        ENDCASE;
    }

    CASE("print_first_dup_pair: 无重复返回 0");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("a b c d e f g h", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("x y z w p q r s", toks1, ANALYZE_MAX_TOKENS);
        AnalyzeFunc funcs[2] = {
            {"fa", "ma", NULL, 0, 0},
            {"fb", "mb", NULL, 0, 0},
        };
        suppress_stdout();
        int r = analyze_print_first_dup_pair(all_toks, tok_counts, 0, 1, funcs);
        restore_stdout();
        ASSERT_EQ_INT(r, 0, "无重复返回 0");
        ENDCASE;
    }

    CASE("print_first_dup_pair: 常见模式跳过");
    {
        /* 构造 for 循环模板（常见模式），不应被报告为重复 */
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("for ( int i = 0 ; i < n ; i ++ ) {", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("for ( int i = 0 ; i < n ; i ++ ) {", toks1, ANALYZE_MAX_TOKENS);
        AnalyzeFunc funcs[2] = {
            {"fa", "ma", NULL, 0, 0},
            {"fb", "mb", NULL, 0, 0},
        };
        suppress_stdout();
        int r = analyze_print_first_dup_pair(all_toks, tok_counts, 0, 1, funcs);
        restore_stdout();
        ASSERT_EQ_INT(r, 0, "常见模式应跳过");
        ENDCASE;
    }

    CASE("print_dup_fragments: 正常不崩溃");
    {
        AnalyzeToken toks0[ANALYZE_MAX_TOKENS];
        AnalyzeToken toks1[ANALYZE_MAX_TOKENS];
        AnalyzeToken *all_toks[2] = {toks0, toks1};
        int tok_counts[2];
        tok_counts[0] = analyze_tokenize("a b c d e f g h i j", toks0, ANALYZE_MAX_TOKENS);
        tok_counts[1] = analyze_tokenize("a b c d e f g h i j", toks1, ANALYZE_MAX_TOKENS);
        AnalyzeFunc funcs[2] = {
            {"fa", "ma", NULL, 0, 0},
            {"fb", "mb", NULL, 0, 0},
        };
        suppress_stdout();
        analyze_print_dup_fragments(all_toks, tok_counts, funcs, 2);
        restore_stdout();
        ASSERT_TRUE(1, "不崩溃");
        ENDCASE;
    }

    CASE("commands_analyze_duplication: 正常不崩溃");
    {
        AnalyzeFunc funcs[2];
        funcs[0].name[0] = 'f'; funcs[0].name[1] = 'a'; funcs[0].name[2] = '\0';
        funcs[0].module[0] = 'm'; funcs[0].module[1] = '\0';
        funcs[0].code = (char*)"a b c d e f g h i j;";
        funcs[0].complexity = 1; funcs[0].call_count = 0;
        funcs[1].name[0] = 'f'; funcs[1].name[1] = 'b'; funcs[1].name[2] = '\0';
        funcs[1].module[0] = 'm'; funcs[1].module[1] = '\0';
        funcs[1].code = (char*)"a b c d e f g h i j;";
        funcs[1].complexity = 1; funcs[1].call_count = 0;
        suppress_stdout();
        commands_analyze_duplication(funcs, 2);
        restore_stdout();
        ASSERT_TRUE(1, "不崩溃");
        ENDCASE;
    }

    CASE("commands_analyze_duplication: 空函数列表");
    {
        suppress_stdout();
        commands_analyze_duplication(NULL, 0);
        restore_stdout();
        ASSERT_TRUE(1, "空列表不崩溃");
        ENDCASE;
    }
}

/* ================================================================== */
/* 25. commands_analyze_code_lines / commands_analyze_cyclomatic      */
/* ================================================================== */
TEST_SUITE(test_print_summaries) {
    CASE("code_lines: 正常计算并打印");
    {
        AnalyzeMod mods[2];
        strncpy(mods[0].name, "mod1", 127); mods[0].name[127] = '\0';
        mods[0].code = "int x;\nint y;\n";
        mods[1].name[0] = 'm'; mods[1].name[1] = 'o'; mods[1].name[2] = 'd';
        mods[1].name[3] = '2'; mods[1].name[4] = '\0';
        mods[1].code = "// comment\nint z;\n";
        suppress_stdout();
        commands_analyze_code_lines(mods, 2);
        restore_stdout();
        ASSERT_EQ_INT(mods[0].code_lines, 2, "mod1 2 行");
        ASSERT_EQ_INT(mods[1].code_lines, 1, "mod2 1 行（注释不计）");
        ENDCASE;
    }

    CASE("code_lines: 空模块列表");
    {
        suppress_stdout();
        commands_analyze_code_lines(NULL, 0);
        restore_stdout();
        ASSERT_TRUE(1, "空列表不崩溃");
        ENDCASE;
    }

    CASE("cyclomatic: 计算并打印");
    {
        AnalyzeFunc funcs[2];
        funcs[0].name[0] = 'f'; funcs[0].name[1] = '\0';
        funcs[0].module[0] = 'm'; funcs[0].module[1] = '\0';
        funcs[0].code = (char*)"if (x) {}";
        funcs[0].complexity = 0; funcs[0].call_count = 0;
        funcs[1].name[0] = 'g'; funcs[1].name[1] = '\0';
        funcs[1].module[0] = 'm'; funcs[1].module[1] = '\0';
        funcs[1].code = (char*)"for (;;) {}";
        funcs[1].complexity = 0; funcs[1].call_count = 0;
        suppress_stdout();
        commands_analyze_cyclomatic(funcs, 2);
        restore_stdout();
        ASSERT_EQ_INT(funcs[0].complexity, 2, "f: if → cc=2");
        ASSERT_EQ_INT(funcs[1].complexity, 2, "g: for → cc=2");
        ENDCASE;
    }

    CASE("cyclomatic: 空函数列表");
    {
        suppress_stdout();
        commands_analyze_cyclomatic(NULL, 0);
        restore_stdout();
        ASSERT_TRUE(1, "空列表不崩溃");
        ENDCASE;
    }
}

/* ================================================================== */
/* 26. commands_analyze_ensure_loaded                                 */
/* ================================================================== */
TEST_SUITE(test_ensure_loaded) {
    CASE("ensure_loaded: root 有子模块时不加载");
    {
        reset_proj();
        add_mod("m", "int x;\n");
        g_mock_parser_calls = 0;
        commands_analyze_ensure_loaded();
        ASSERT_EQ_INT(g_mock_parser_calls, 0, "有子模块不调用 parser");
        ENDCASE;
    }

    CASE("ensure_loaded: 无子模块无 .cboot 不加载");
    {
        reset_proj();
        const char *dir = "/tmp/test_ensure_1";
        enter_tmp(dir);
        /* 不创建 .cboot */
        g_mock_parser_calls = 0;
        commands_analyze_ensure_loaded();
        ASSERT_EQ_INT(g_mock_parser_calls, 0, "无 .cboot 不调用 parser");
        leave_tmp(dir);
        ENDCASE;
    }

    CASE("ensure_loaded: 无子模块有 .cboot 触发加载");
    {
        reset_proj();
        const char *dir = "/tmp/test_ensure_2";
        enter_tmp(dir);
        write_temp_file(".cboot", "project test\n");
        g_mock_parser_calls = 0;
        g_mock_parser_return = 0;
        commands_analyze_ensure_loaded();
        ASSERT_TRUE(g_mock_parser_calls > 0, "有 .cboot 应调用 parser");
        ASSERT_NOT_NULL(g_proj, "g_proj 应被重建");
        ASSERT_NOT_NULL(g_proj->root, "root 应存在");
        ASSERT_EQ_INT(g_skip_gen, 0, "g_skip_gen 应恢复为 0");
        leave_tmp(dir);
        ENDCASE;
    }
}

/* ================================================================== */
/* 27. commands_cmd_analyze_impl (公共入口)                            */
/* ================================================================== */
TEST_SUITE(test_cmd_analyze_impl) {
    CASE("impl: g_proj NULL 返回 -1");
    {
        Project *sv = g_proj;
        g_proj = NULL;
        suppress_stdout();
        int rc = commands_cmd_analyze_impl();
        restore_stdout();
        ASSERT_EQ_INT(rc, -1, "NULL g_proj → -1");
        g_proj = sv;
        ENDCASE;
    }

    CASE("impl: root NULL 返回 -1");
    {
        reset_proj();
        Domain *r = g_proj->root;
        g_proj->root = NULL;
        suppress_stdout();
        int rc = commands_cmd_analyze_impl();
        restore_stdout();
        ASSERT_EQ_INT(rc, -1, "NULL root → -1");
        g_proj->root = r;
        ENDCASE;
    }

    CASE("impl: 空项目返回 0");
    {
        reset_proj();
        const char *dir = "/tmp/test_impl_empty";
        enter_tmp(dir);
        suppress_stdout();
        int rc = commands_cmd_analyze_impl();
        restore_stdout();
        ASSERT_EQ_INT(rc, 0, "空项目 → 0");
        leave_tmp(dir);
        ENDCASE;
    }

    CASE("impl: 有模块和函数的完整分析");
    {
        reset_proj();
        const char *dir = "/tmp/test_impl_full";
        enter_tmp(dir);

        /* 创建临时 .c 文件 */
        const char *code1 =
            "int add(int a, int b) {\n"
            "    return a + b;\n"
            "}\n"
            "int sub(int a, int b) {\n"
            "    return a - b;\n"
            "}\n";
        const char *code2 =
            "int compute(int x, int y) {\n"
            "    int result = add(x, y);\n"
            "    result = sub(result, y);\n"
            "    if (result > 0) {\n"
            "        return result;\n"
            "    }\n"
            "    return 0;\n"
            "}\n";
        write_temp_file("mod1.c", code1);
        write_temp_file("mod2.c", code2);

        /* 读取文件内容设为模块 code */
        char *c1 = read_file("mod1.c");
        char *c2 = read_file("mod2.c");
        add_mod("mod1", c1);
        add_mod("mod2", c2);
        free(c1);
        free(c2);

        suppress_stdout();
        int rc = commands_cmd_analyze_impl();
        restore_stdout();
        ASSERT_EQ_INT(rc, 0, "完整分析 → 0");

        leave_tmp(dir);
        ENDCASE;
    }

    CASE("impl: 带重复代码的模块");
    {
        reset_proj();
        const char *dir = "/tmp/test_impl_dup";
        enter_tmp(dir);

        /* 两个模块中有重复的函数体 */
        const char *code1 =
            "int helper(int x) {\n"
            "    int result = x * 2;\n"
            "    return result;\n"
            "}\n";
        const char *code2 =
            "int helper2(int y) {\n"
            "    int result = y * 2;\n"
            "    return result;\n"
            "}\n";
        write_temp_file("a.c", code1);
        write_temp_file("b.c", code2);
        char *c1 = read_file("a.c");
        char *c2 = read_file("b.c");
        add_mod("modA", c1);
        add_mod("modB", c2);
        free(c1);
        free(c2);

        suppress_stdout();
        int rc = commands_cmd_analyze_impl();
        restore_stdout();
        ASSERT_EQ_INT(rc, 0, "重复代码分析 → 0");

        leave_tmp(dir);
        ENDCASE;
    }

    CASE("impl: 通过 commands_cmd_analyze 转发");
    {
        reset_proj();
        const char *dir = "/tmp/test_impl_fwd";
        enter_tmp(dir);
        add_mod("m", "int f() { return 0; }\n");
        suppress_stdout();
        int rc = commands_cmd_analyze();
        restore_stdout();
        ASSERT_EQ_INT(rc, 0, "转发调用 → 0");
        leave_tmp(dir);
        ENDCASE;
    }
}

/* ================================================================== */
/* 主函数                                                             */
/* ================================================================== */
int main(void) {
    getcwd(g_orig_cwd, sizeof(g_orig_cwd));

    /* 全局项目初始化 */
    g_proj = domain_project_new("test");
    g_mode = MODE_INTERACTIVE;
    g_running = 1;

    printf("========== commands/analyze/analyze.c 单元测试 ==========\n");

    RUN_SUITE(test_count_line_end);
    RUN_SUITE(test_count_normal_char);
    RUN_SUITE(test_count_quote_char);
    RUN_SUITE(test_count_states);
    RUN_SUITE(test_count_code_lines);
    RUN_SUITE(test_is_keyword);
    RUN_SUITE(test_skip_functions);
    RUN_SUITE(test_find_body_end);
    RUN_SUITE(test_find_parens);
    RUN_SUITE(test_extract_func_name);
    RUN_SUITE(test_extract_functions);
    RUN_SUITE(test_is_two_char_op);
    RUN_SUITE(test_emit_functions);
    RUN_SUITE(test_skip_trivia);
    RUN_SUITE(test_tokenize);
    RUN_SUITE(test_cyclomatic_complexity);
    RUN_SUITE(test_ngram_equal);
    RUN_SUITE(test_ngram_patterns);
    RUN_SUITE(test_build_module_path);
    RUN_SUITE(test_collect_modules);
    RUN_SUITE(test_has_call);
    RUN_SUITE(test_resolve_call);
    RUN_SUITE(test_dependency);
    RUN_SUITE(test_ngram_dup);
    RUN_SUITE(test_print_summaries);
    RUN_SUITE(test_ensure_loaded);
    RUN_SUITE(test_cmd_analyze_impl);

    /* 清理 */
    chdir(g_orig_cwd);
    if (g_proj) domain_project_free(g_proj);
    g_proj = NULL;

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
