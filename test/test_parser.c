/*
 * test_parser.c - parser/parser.c 单元测试
 * 覆盖 parser.c 中所有函数和逻辑分支
 *
 * 通过 #include "parser/parser.c" 方式包含被测源文件以测试 static 函数。
 * parser.c 依赖: utils, domain/core, domain, typecheck, cupdate, cupdate_lexer, cupdate_parser
 * mock: commands_cmd_* 系列函数 + generator_generate_cboot_only
 */
#include "test.h"

/* Include dependencies (order matters: dependencies first) */
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "cupdate/cupdate.c"
#include "cupdate/cupdate_lexer.c"
#include "cupdate/cupdate_parser.c"

/* ------------------------------------------------------------------ */
/* Global state (declared extern in cboot.h, defined here)             */
/* g_script_dir is defined in parser.c (included below)                */
/* ------------------------------------------------------------------ */
Project *g_proj   = NULL;
RunMode  g_mode   = MODE_BATCH;
int      g_force  = 0;
int      g_running = 1;
int      g_skip_gen = 0;

/* ------------------------------------------------------------------ */
/* Mock generator dependency                                           */
/* ------------------------------------------------------------------ */
int generator_generate_cboot_only(Project *proj) { (void)proj; return 0; }

/* ------------------------------------------------------------------ */
/* Mock state for commands_cmd_* functions                             */
/* ------------------------------------------------------------------ */
static const char *g_mock_last_cmd = "";
static const char *g_mock_last_arg1 = NULL;
static const char *g_mock_last_arg2 = NULL;
static int g_mock_last_int = 0;
static int g_mock_call_count = 0;
static int g_mock_fail = 0;

static void mock_reset(void) {
    g_mock_last_cmd = "";
    g_mock_last_arg1 = NULL;
    g_mock_last_arg2 = NULL;
    g_mock_last_int = 0;
    g_mock_call_count = 0;
    g_mock_fail = 0;
}

/* ------------------------------------------------------------------ */
/* Mock implementations of commands_cmd_* functions                     */
/*                                                                     */
/* commands_cmd_mod 和 commands_cmd_cd 执行真实的域操作（创建模块、     */
/* 导航域树），因为 parser_exec_cboot_ref 依赖它们来创建和进入模块。     */
/* 其他 commands_cmd_* 仅跟踪调用并返回 0（或 g_mock_fail 时返回 -1）。   */
/* ------------------------------------------------------------------ */

int commands_cmd_mod(const char *name) {
    g_mock_last_cmd = "mod";
    g_mock_last_arg1 = name;
    g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    /* Real work: create module domain */
    ModuleDomain *mod = domain_module_domain_new(name);
    if (!mod) return -1;
    domain_domain_add_child(g_proj->current, (Domain *)mod);
    return 0;
}

int commands_cmd_cd(const char *path) {
    g_mock_last_cmd = "cd";
    g_mock_last_arg1 = path;
    g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    /* Real work: navigate domain tree */
    if (!path || path[0] == '\0') return 0;
    if (utils_str_eq(path, "..")) {
        if (g_proj->current->parent) g_proj->current = g_proj->current->parent;
        return 0;
    }
    Domain *child = domain_domain_find_child(g_proj->current, path);
    if (!child) return -1;
    g_proj->current = child;
    return 0;
}

int commands_cmd_struct(const char *name) {
    g_mock_last_cmd = "struct"; g_mock_last_arg1 = name; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_type(const char *name) {
    g_mock_last_cmd = "type"; g_mock_last_arg1 = name; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_def(const char *name) {
    g_mock_last_cmd = "def"; g_mock_last_arg1 = name; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_void(const char *name, const char *return_type) {
    g_mock_last_cmd = "void"; g_mock_last_arg1 = name;
    g_mock_last_arg2 = return_type; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_var(const char *name, const char *type) {
    g_mock_last_cmd = "var"; g_mock_last_arg1 = name;
    g_mock_last_arg2 = type; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_mem(const char *name, const char *type) {
    g_mock_last_cmd = "mem"; g_mock_last_arg1 = name;
    g_mock_last_arg2 = type; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_enum(const char *defs, const char *start_num_str) {
    g_mock_last_cmd = "enum"; g_mock_last_arg1 = defs;
    g_mock_last_arg2 = start_num_str; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_cmt(const char *text) {
    g_mock_last_cmd = "cmt"; g_mock_last_arg1 = text; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_value(const char *text) {
    g_mock_last_cmd = "value"; g_mock_last_arg1 = text; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_call(const char *call_conv) {
    g_mock_last_cmd = "call"; g_mock_last_arg1 = call_conv; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_mode(const char *text) {
    g_mock_last_cmd = "mode"; g_mock_last_arg1 = text; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_cmode(const char *text) {
    g_mock_last_cmd = "cmode"; g_mock_last_arg1 = text; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_rm(const char *name, int force) {
    g_mock_last_cmd = "rm"; g_mock_last_arg1 = name;
    g_mock_last_int = force; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_find(const char *type_filter, const char *pattern, int flags) {
    g_mock_last_cmd = "find"; g_mock_last_arg1 = type_filter;
    g_mock_last_arg2 = pattern; g_mock_last_int = flags; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_ls(const char *name) {
    g_mock_last_cmd = "ls"; g_mock_last_arg1 = name; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_mv(const char *src, const char *target) {
    g_mock_last_cmd = "mv"; g_mock_last_arg1 = src;
    g_mock_last_arg2 = target; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_gen(void) {
    g_mock_last_cmd = "gen"; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_update(void) {
    g_mock_last_cmd = "update"; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_analyze(void) {
    g_mock_last_cmd = "analyze"; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_im(const char *path) {
    g_mock_last_cmd = "im"; g_mock_last_arg1 = path; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_in(const char *path) {
    g_mock_last_cmd = "in"; g_mock_last_arg1 = path; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

int commands_cmd_res(const char *file_path) {
    g_mock_last_cmd = "res"; g_mock_last_arg1 = file_path; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}

/* Include the file under test (after mocks are defined) */
#include "parser/parser.c"

/* ------------------------------------------------------------------ */
/* Helper functions                                                    */
/* ------------------------------------------------------------------ */

static void test_setup(void) {
    if (g_proj) domain_project_free(g_proj);
    g_proj = domain_project_new("test");
    g_mode = MODE_BATCH;
    g_running = 1;
    g_force = 0;
    g_skip_gen = 0;
    strcpy(g_script_dir, ".");
    mock_reset();
}

static int write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

/* Create tokens from a line; caller must free with utils_free_tokens */
static char **make_tokens(const char *line, int *count) {
    return tokenize(line, count);
}

/* ================================================================== */
/* parser_is_cboot_ref (static)                                        */
/* ================================================================== */

TEST_SUITE(test_is_cboot_ref_null) {
    TEST_BEGIN("is_cboot_ref NULL");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref(NULL), 0, "NULL 应返回 0");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_exact) {
    TEST_BEGIN("is_cboot_ref \".cboot\"");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref(".cboot"), 1, ".cboot 应识别");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_single_level) {
    TEST_BEGIN("is_cboot_ref \"b/.cboot\"");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref("b/.cboot"), 1, "b/.cboot 应识别");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_multi_level) {
    TEST_BEGIN("is_cboot_ref \"b/c/.cboot\"");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref("b/c/.cboot"), 1, "b/c/.cboot 应识别");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_not_ref) {
    TEST_BEGIN("is_cboot_ref 普通命令");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref("mod"), 0, "mod 不是引用");
    ASSERT_EQ_INT(parser_is_cboot_ref("gen"), 0, "gen 不是引用");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_too_short) {
    TEST_BEGIN("is_cboot_ref 过短字符串");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref(".cboo"), 0, ".cboo 过短");
    ASSERT_EQ_INT(parser_is_cboot_ref("abc"), 0, "abc 过短");
    ASSERT_EQ_INT(parser_is_cboot_ref(""), 0, "空串过短");
    TEST_END();
}

TEST_SUITE(test_is_cboot_ref_long_ref) {
    TEST_BEGIN("is_cboot_ref 长路径引用");
    test_setup();
    ASSERT_EQ_INT(parser_is_cboot_ref("a/b/c/d/.cboot"), 1, "多级路径应识别");
    ASSERT_EQ_INT(parser_is_cboot_ref("x.cboot"), 1, "x.cboot 应识别");
    TEST_END();
}

/* ================================================================== */
/* parser_try_cboot_ref (public)                                       */
/* ================================================================== */

TEST_SUITE(test_try_cboot_ref_not_ref) {
    TEST_BEGIN("try_cboot_ref 非引用返回 0");
    test_setup();
    ASSERT_EQ_INT(parser_try_cboot_ref("mod"), 0, "非引用应返回 0");
    ASSERT_EQ_INT(g_mock_call_count, 0, "不应执行任何命令");
    TEST_END();
}

TEST_SUITE(test_try_cboot_ref_is_ref) {
    TEST_BEGIN("try_cboot_ref 引用返回 1");
    test_setup();
    /* 创建临时 .cboot 文件供 parser_exec_cboot_ref 解析 */
    write_temp_file("/tmp/.cboot", "# test\n");
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    ASSERT_EQ_INT(parser_try_cboot_ref(".cboot"), 1, "引用应返回 1");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/.cboot");
    TEST_END();
}

TEST_SUITE(test_try_cboot_ref_null) {
    TEST_BEGIN("try_cboot_ref NULL");
    test_setup();
    ASSERT_EQ_INT(parser_try_cboot_ref(NULL), 0, "NULL 应返回 0");
    TEST_END();
}

/* ================================================================== */
/* parser_exec_cboot_ref (static)                                      */
/* ================================================================== */

TEST_SUITE(test_exec_cboot_ref_null) {
    TEST_BEGIN("exec_cboot_ref NULL");
    test_setup();
    ASSERT_EQ_INT(parser_exec_cboot_ref(NULL), -1, "NULL 应返回 -1");
    TEST_END();
}

TEST_SUITE(test_exec_cboot_ref_no_slash) {
    TEST_BEGIN("exec_cboot_ref 无路径（.cboot）");
    test_setup();
    write_temp_file("/tmp/.cboot", "# empty ref\n");
    Domain *saved = g_proj->current;
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    int rc = parser_exec_cboot_ref(".cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_TRUE(g_proj->current == saved, "作用域应恢复");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/.cboot");
    TEST_END();
}

TEST_SUITE(test_exec_cboot_ref_single_level) {
    TEST_BEGIN("exec_cboot_ref 单级路径（b/.cboot）");
    test_setup();
    system("mkdir -p /tmp/pt_b");
    write_temp_file("/tmp/pt_b/.cboot", "# single level\n");
    Domain *saved = g_proj->current;
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    int rc = parser_exec_cboot_ref("pt_b/.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_TRUE(g_proj->current == saved, "作用域应恢复");
    /* 验证模块被创建 */
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "pt_b"), "模块 pt_b 应创建");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/pt_b/.cboot");
    system("rmdir /tmp/pt_b");
    TEST_END();
}

TEST_SUITE(test_exec_cboot_ref_multi_level) {
    TEST_BEGIN("exec_cboot_ref 多级路径（b/c/.cboot）");
    test_setup();
    system("mkdir -p /tmp/pt_b/pt_c");
    write_temp_file("/tmp/pt_b/pt_c/.cboot", "# multi level\n");
    Domain *saved = g_proj->current;
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    int rc = parser_exec_cboot_ref("pt_b/pt_c/.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_TRUE(g_proj->current == saved, "作用域应恢复");
    /* 验证多级模块被创建 */
    Domain *b = domain_domain_find_child(g_proj->root, "pt_b");
    ASSERT_NOT_NULL(b, "模块 pt_b 应创建");
    Domain *c = domain_domain_find_child(b, "pt_c");
    ASSERT_NOT_NULL(c, "模块 pt_c 应创建");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/pt_b/pt_c/.cboot");
    system("rmdir /tmp/pt_b/pt_c");
    system("rmdir /tmp/pt_b");
    TEST_END();
}

TEST_SUITE(test_exec_cboot_ref_mod_fail) {
    TEST_BEGIN("exec_cboot_ref mod 失败回滚");
    test_setup();
    system("mkdir -p /tmp/pt_fail");
    write_temp_file("/tmp/pt_fail/.cboot", "# fail test\n");
    Domain *saved = g_proj->current;
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    g_mock_fail = 1;
    int rc = parser_exec_cboot_ref("pt_fail/.cboot");
    ASSERT_EQ_INT(rc, -1, "mod 失败应返回 -1");
    ASSERT_TRUE(g_proj->current == saved, "作用域应恢复");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/pt_fail/.cboot");
    system("rmdir /tmp/pt_fail");
    TEST_END();
}

TEST_SUITE(test_exec_cboot_ref_file_not_found) {
    TEST_BEGIN("exec_cboot_ref 文件不存在");
    test_setup();
    Domain *saved = g_proj->current;
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    int rc = parser_exec_cboot_ref("nonexist_dir/.cboot");
    ASSERT_EQ_INT(rc, -1, "文件不存在应返回 -1");
    ASSERT_TRUE(g_proj->current == saved, "作用域应恢复");
    strcpy(g_script_dir, saved_dir);
    TEST_END();
}

/* ================================================================== */
/* parser_join_tokens_from (static)                                    */
/* ================================================================== */

TEST_SUITE(test_join_tokens_empty) {
    TEST_BEGIN("join_tokens_from start >= count");
    test_setup();
    char *toks[] = {"a", "b", "c"};
    char buf[256];
    parser_join_tokens_from(toks, 3, 5, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "", "start >= count 应为空串");
    TEST_END();
}

TEST_SUITE(test_join_tokens_single) {
    TEST_BEGIN("join_tokens_from 单 token");
    test_setup();
    char *toks[] = {"cmd", "arg1", "arg2"};
    char buf[256];
    parser_join_tokens_from(toks, 2, 1, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "arg1", "应拼接单个 token");
    TEST_END();
}

TEST_SUITE(test_join_tokens_multiple) {
    TEST_BEGIN("join_tokens_from 多 token");
    test_setup();
    char *toks[] = {"cmd", "arg1", "arg2", "arg3"};
    char buf[256];
    parser_join_tokens_from(toks, 4, 1, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "arg1 arg2 arg3", "应用空格拼接");
    TEST_END();
}

TEST_SUITE(test_join_tokens_from_zero) {
    TEST_BEGIN("join_tokens_from start=0");
    test_setup();
    char *toks[] = {"hello", "world"};
    char buf[256];
    parser_join_tokens_from(toks, 2, 0, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "hello world", "start=0 应包含所有 token");
    TEST_END();
}

/* ================================================================== */
/* parser_require_args (static)                                        */
/* ================================================================== */

TEST_SUITE(test_require_args_sufficient) {
    TEST_BEGIN("require_args 参数足够");
    test_setup();
    char *toks[] = {"cmd", "a", "b"};
    ASSERT_EQ_INT(parser_require_args(toks, 3, 2, "usage"), 0, "3>=2 应返回 0");
    TEST_END();
}

TEST_SUITE(test_require_args_insufficient) {
    TEST_BEGIN("require_args 参数不足");
    test_setup();
    char *toks[] = {"cmd"};
    ASSERT_EQ_INT(parser_require_args(toks, 1, 2, "usage"), -1, "1<2 应返回 -1");
    TEST_END();
}

TEST_SUITE(test_require_args_exact) {
    TEST_BEGIN("require_args 参数刚好");
    test_setup();
    char *toks[] = {"cmd", "a"};
    ASSERT_EQ_INT(parser_require_args(toks, 2, 2, "usage"), 0, "2>=2 应返回 0");
    TEST_END();
}

/* ================================================================== */
/* parser_dispatch_build (static)                                      */
/* ================================================================== */

TEST_SUITE(test_dispatch_build_project) {
    TEST_BEGIN("dispatch_build project 命令");
    test_setup();
    int count;
    char **toks = make_tokens("project newname", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_proj->name, "newname", "项目名应更新");
    ASSERT_EQ_STR(g_proj->root->name, "newname", "根域名应更新");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_project_no_args) {
    TEST_BEGIN("dispatch_build project 无参数");
    test_setup();
    char *toks[] = {"project"};
    int rc = parser_dispatch_build(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_build_mod) {
    TEST_BEGIN("dispatch_build mod 命令");
    test_setup();
    int count;
    char **toks = make_tokens("mod mymod", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "mod", "应调用 mod");
    ASSERT_EQ_STR(g_mock_last_arg1, "mymod", "参数应为 mymod");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_mod_no_args) {
    TEST_BEGIN("dispatch_build mod 无参数");
    test_setup();
    char *toks[] = {"mod"};
    int rc = parser_dispatch_build(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    ASSERT_EQ_INT(g_mock_call_count, 0, "不应调用 mod");
    TEST_END();
}

TEST_SUITE(test_dispatch_build_struct) {
    TEST_BEGIN("dispatch_build struct 命令");
    test_setup();
    int count;
    char **toks = make_tokens("struct Point", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "struct", "应调用 struct");
    ASSERT_EQ_STR(g_mock_last_arg1, "Point", "参数应为 Point");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_type) {
    TEST_BEGIN("dispatch_build type 命令");
    test_setup();
    int count;
    char **toks = make_tokens("type MyType", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "type", "应调用 type");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_def) {
    TEST_BEGIN("dispatch_build def 命令");
    test_setup();
    int count;
    char **toks = make_tokens("def MAX", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "def", "应调用 def");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_void) {
    TEST_BEGIN("dispatch_build void 命令（拼接类型）");
    test_setup();
    int count;
    char **toks = make_tokens("void func int", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "void", "应调用 void");
    ASSERT_EQ_STR(g_mock_last_arg1, "func", "name 应为 func");
    ASSERT_EQ_STR(g_mock_last_arg2, "int", "type 应为 int");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_void_multi_type) {
    TEST_BEGIN("dispatch_build void 多 token 类型拼接");
    test_setup();
    int count;
    char **toks = make_tokens("void func unsigned int", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_arg2, "unsigned int", "type 应拼接");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_void_no_args) {
    TEST_BEGIN("dispatch_build void 参数不足");
    test_setup();
    char *toks[] = {"void", "func"};
    int rc = parser_dispatch_build(toks, 2);
    ASSERT_EQ_INT(rc, -1, "参数不足应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_build_var) {
    TEST_BEGIN("dispatch_build var 命令");
    test_setup();
    int count;
    char **toks = make_tokens("var count int", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "var", "应调用 var");
    ASSERT_EQ_STR(g_mock_last_arg1, "count", "name");
    ASSERT_EQ_STR(g_mock_last_arg2, "int", "type");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_mem) {
    TEST_BEGIN("dispatch_build mem 命令");
    test_setup();
    int count;
    char **toks = make_tokens("mem x int", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "mem", "应调用 mem");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_enum) {
    TEST_BEGIN("dispatch_build enum 命令");
    test_setup();
    int count;
    char **toks = make_tokens("enum A,B 0", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "enum", "应调用 enum");
    ASSERT_EQ_STR(g_mock_last_arg1, "A,B", "defs");
    ASSERT_EQ_STR(g_mock_last_arg2, "0", "start");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_build_enum_no_args) {
    TEST_BEGIN("dispatch_build enum 参数不足");
    test_setup();
    char *toks[] = {"enum", "A,B"};
    int rc = parser_dispatch_build(toks, 2);
    ASSERT_EQ_INT(rc, -1, "参数不足应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_build_unknown) {
    TEST_BEGIN("dispatch_build 未知命令");
    test_setup();
    char *toks[] = {"unknown_cmd"};
    int rc = parser_dispatch_build(toks, 1);
    ASSERT_EQ_INT(rc, PARSER_NOT_HANDLED, "未知命令应返回 NOT_HANDLED");
    TEST_END();
}

TEST_SUITE(test_dispatch_build_mod_fail) {
    TEST_BEGIN("dispatch_build mod 返回失败值");
    test_setup();
    g_mock_fail = 1;
    int count;
    char **toks = make_tokens("mod failmod", &count);
    int rc = parser_dispatch_build(toks, count);
    ASSERT_EQ_INT(rc, -1, "mod 失败应返回 -1");
    utils_free_tokens(toks, count);
    TEST_END();
}

/* ================================================================== */
/* parser_dispatch_modify (static)                                     */
/* ================================================================== */

TEST_SUITE(test_dispatch_modify_value) {
    TEST_BEGIN("dispatch_modify value 命令");
    test_setup();
    int count;
    char **toks = make_tokens("value 42", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "value", "应调用 value");
    ASSERT_EQ_STR(g_mock_last_arg1, "42", "参数应为 42");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_value_no_args) {
    TEST_BEGIN("dispatch_modify value 无参数");
    test_setup();
    char *toks[] = {"value"};
    int rc = parser_dispatch_modify(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_cmode) {
    TEST_BEGIN("dispatch_modify cmode 命令");
    test_setup();
    int count;
    char **toks = make_tokens("cmode exe", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "cmode", "应调用 cmode");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_cmt) {
    TEST_BEGIN("dispatch_modify cmt 命令（去引号）");
    test_setup();
    int count;
    char **toks = make_tokens("cmt \"hello world\"", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "cmt", "应调用 cmt");
    ASSERT_EQ_STR(g_mock_last_arg1, "hello world", "应去除引号");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_cmt_no_args) {
    TEST_BEGIN("dispatch_modify cmt 无参数");
    test_setup();
    char *toks[] = {"cmt"};
    int rc = parser_dispatch_modify(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_call) {
    TEST_BEGIN("dispatch_modify call 命令");
    test_setup();
    int count;
    char **toks = make_tokens("call __stdcall", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "call", "应调用 call");
    ASSERT_EQ_STR(g_mock_last_arg1, "__stdcall", "参数");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_mode) {
    TEST_BEGIN("dispatch_modify mode 命令");
    test_setup();
    int count;
    char **toks = make_tokens("mode api", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "mode", "应调用 mode");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_code_inline) {
    TEST_BEGIN("dispatch_modify code 内联设置");
    test_setup();
    int count;
    char **toks = make_tokens("code return 0;", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_NOT_NULL(mod->code, "code 应已设置");
    ASSERT_EQ_STR(mod->code, "return 0;", "code 内容应匹配");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_code_heredoc_signal) {
    TEST_BEGIN("dispatch_modify code <<EOF 返回 1");
    test_setup();
    int count;
    char **toks = make_tokens("code <<EOF", &count);
    int rc = parser_dispatch_modify(toks, count);
    ASSERT_EQ_INT(rc, 1, "code <<EOF 应返回 1（heredoc 信号）");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_code_alone) {
    TEST_BEGIN("dispatch_modify code 单独返回 1");
    test_setup();
    char *toks[] = {"code"};
    int rc = parser_dispatch_modify(toks, 1);
    ASSERT_EQ_INT(rc, 1, "code 单独应返回 1（heredoc 信号）");
    TEST_END();
}

TEST_SUITE(test_dispatch_modify_unknown) {
    TEST_BEGIN("dispatch_modify 未知命令");
    test_setup();
    char *toks[] = {"unknown_cmd"};
    int rc = parser_dispatch_modify(toks, 1);
    ASSERT_EQ_INT(rc, PARSER_NOT_HANDLED, "未知命令应返回 NOT_HANDLED");
    TEST_END();
}

/* ================================================================== */
/* parser_dispatch_control (static)                                    */
/* ================================================================== */

TEST_SUITE(test_dispatch_control_cd_with_path) {
    TEST_BEGIN("dispatch_control cd 带路径");
    test_setup();
    /* 先创建一个子模块 */
    commands_cmd_mod("submod");
    g_mock_call_count = 0; /* 重置计数 */
    int count;
    char **toks = make_tokens("cd submod", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "cd", "应调用 cd");
    ASSERT_EQ_STR(g_mock_last_arg1, "submod", "路径");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_cd_no_path) {
    TEST_BEGIN("dispatch_control cd 无路径");
    test_setup();
    char *toks[] = {"cd"};
    int rc = parser_dispatch_control(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "cd", "应调用 cd");
    ASSERT_NULL(g_mock_last_arg1, "路径应为 NULL");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_ls_with_name) {
    TEST_BEGIN("dispatch_control ls 带名称");
    test_setup();
    int count;
    char **toks = make_tokens("ls submod", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "ls", "应调用 ls");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_ls_no_name) {
    TEST_BEGIN("dispatch_control ls 无名称");
    test_setup();
    char *toks[] = {"ls"};
    int rc = parser_dispatch_control(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "ls", "应调用 ls");
    ASSERT_NULL(g_mock_last_arg1, "名称应为 NULL");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_exit) {
    TEST_BEGIN("dispatch_control exit 命令");
    test_setup();
    g_running = 1;
    char *toks[] = {"exit"};
    int rc = parser_dispatch_control(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(g_running, 0, "g_running 应设为 0");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_mv) {
    TEST_BEGIN("dispatch_control mv 命令");
    test_setup();
    int count;
    char **toks = make_tokens("mv src tgt", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "mv", "应调用 mv");
    ASSERT_EQ_STR(g_mock_last_arg1, "src", "src");
    ASSERT_EQ_STR(g_mock_last_arg2, "tgt", "target");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_mv_no_args) {
    TEST_BEGIN("dispatch_control mv 参数不足");
    test_setup();
    char *toks[] = {"mv", "src"};
    int rc = parser_dispatch_control(toks, 2);
    ASSERT_EQ_INT(rc, -1, "参数不足应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_rm) {
    TEST_BEGIN("dispatch_control rm 命令");
    test_setup();
    int count;
    char **toks = make_tokens("rm myname", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "rm", "应调用 rm");
    ASSERT_EQ_STR(g_mock_last_arg1, "myname", "name");
    ASSERT_EQ_INT(g_mock_last_int, 0, "force 应为 0");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_rm_force) {
    TEST_BEGIN("dispatch_control rm -f 命令");
    test_setup();
    int count;
    char **toks = make_tokens("rm -f myname", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "rm", "应调用 rm");
    ASSERT_EQ_STR(g_mock_last_arg1, "myname", "name");
    ASSERT_EQ_INT(g_mock_last_int, 1, "force 应为 1");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_rm_no_name) {
    TEST_BEGIN("dispatch_control rm 无名称");
    test_setup();
    char *toks[] = {"rm"};
    int rc = parser_dispatch_control(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无名称应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_find) {
    TEST_BEGIN("dispatch_control find 命令");
    test_setup();
    int count;
    char **toks = make_tokens("find mod abc", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "find", "应调用 find");
    ASSERT_EQ_STR(g_mock_last_arg1, "mod", "type");
    ASSERT_EQ_STR(g_mock_last_arg2, "abc", "pattern");
    ASSERT_EQ_INT(g_mock_last_int, 0, "flags 应为 0");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_find_with_flags) {
    TEST_BEGIN("dispatch_control find 带 -a 标志");
    test_setup();
    int count;
    char **toks = make_tokens("find mod abc -a", &count);
    int rc = parser_dispatch_control(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(g_mock_last_int, 1, "flags 应为 1");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_control_find_no_args) {
    TEST_BEGIN("dispatch_control find 参数不足");
    test_setup();
    char *toks[] = {"find", "mod"};
    int rc = parser_dispatch_control(toks, 2);
    ASSERT_EQ_INT(rc, -1, "参数不足应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_control_unknown) {
    TEST_BEGIN("dispatch_control 未知命令");
    test_setup();
    char *toks[] = {"unknown_cmd"};
    int rc = parser_dispatch_control(toks, 1);
    ASSERT_EQ_INT(rc, PARSER_NOT_HANDLED, "未知命令应返回 NOT_HANDLED");
    TEST_END();
}

/* ================================================================== */
/* parser_dispatch_action (static)                                     */
/* ================================================================== */

TEST_SUITE(test_dispatch_action_gen) {
    TEST_BEGIN("dispatch_action gen 命令");
    test_setup();
    g_skip_gen = 0;
    char *toks[] = {"gen"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "gen", "应调用 gen");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_gen_skip) {
    TEST_BEGIN("dispatch_action gen 跳过（g_skip_gen=1）");
    test_setup();
    g_skip_gen = 1;
    g_mock_call_count = 0;
    char *toks[] = {"gen"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(g_mock_call_count, 0, "不应调用 gen");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_analyze) {
    TEST_BEGIN("dispatch_action analyze 命令");
    test_setup();
    char *toks[] = {"analyze"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "analyze", "应调用 analyze");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_update) {
    TEST_BEGIN("dispatch_action update 命令返回 2");
    test_setup();
    char *toks[] = {"update"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, 2, "update 应始终返回 2");
    ASSERT_EQ_STR(g_mock_last_cmd, "update", "应调用 update");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_im) {
    TEST_BEGIN("dispatch_action im 命令");
    test_setup();
    int count;
    char **toks = make_tokens("im some.cboot", &count);
    int rc = parser_dispatch_action(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "im", "应调用 im");
    ASSERT_EQ_STR(g_mock_last_arg1, "some.cboot", "path");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_action_im_no_args) {
    TEST_BEGIN("dispatch_action im 无参数");
    test_setup();
    char *toks[] = {"im"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_in) {
    TEST_BEGIN("dispatch_action in 命令");
    test_setup();
    int count;
    char **toks = make_tokens("in proj.cboot", &count);
    int rc = parser_dispatch_action(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "in", "应调用 in");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_action_in_no_args) {
    TEST_BEGIN("dispatch_action in 无参数");
    test_setup();
    char *toks[] = {"in"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_res) {
    TEST_BEGIN("dispatch_action res 命令");
    test_setup();
    int count;
    char **toks = make_tokens("res logo.png", &count);
    int rc = parser_dispatch_action(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "res", "应调用 res");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_action_res_no_args) {
    TEST_BEGIN("dispatch_action res 无参数");
    test_setup();
    char *toks[] = {"res"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, -1, "无参数应返回 -1");
    TEST_END();
}

TEST_SUITE(test_dispatch_action_unknown) {
    TEST_BEGIN("dispatch_action 未知命令");
    test_setup();
    char *toks[] = {"unknown_cmd"};
    int rc = parser_dispatch_action(toks, 1);
    ASSERT_EQ_INT(rc, PARSER_NOT_HANDLED, "未知命令应返回 NOT_HANDLED");
    TEST_END();
}

/* ================================================================== */
/* parser_dispatch_script_line (static)                                */
/* ================================================================== */

TEST_SUITE(test_dispatch_script_line_build) {
    TEST_BEGIN("dispatch_script_line build 命令");
    test_setup();
    int count;
    char **toks = make_tokens("mod mymod", &count);
    int rc = parser_dispatch_script_line(toks, count);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "mod", "应调用 mod");
    utils_free_tokens(toks, count);
    TEST_END();
}

TEST_SUITE(test_dispatch_script_line_action) {
    TEST_BEGIN("dispatch_script_line action 命令");
    test_setup();
    char *toks[] = {"analyze"};
    int rc = parser_dispatch_script_line(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "analyze", "应调用 analyze");
    TEST_END();
}

TEST_SUITE(test_dispatch_script_line_control) {
    TEST_BEGIN("dispatch_script_line control 命令");
    test_setup();
    char *toks[] = {"exit"};
    int rc = parser_dispatch_script_line(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(g_running, 0, "exit 应设置 g_running=0");
    TEST_END();
}

TEST_SUITE(test_dispatch_script_line_cboot_ref) {
    TEST_BEGIN("dispatch_script_line .cboot 引用");
    test_setup();
    write_temp_file("/tmp/.cboot", "# ref test\n");
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    char *toks[] = {".cboot"};
    int rc = parser_dispatch_script_line(toks, 1);
    ASSERT_EQ_INT(rc, 0, "应返回 0（引用失败也返回 0）");
    strcpy(g_script_dir, saved_dir);
    remove("/tmp/.cboot");
    TEST_END();
}

TEST_SUITE(test_dispatch_script_line_cboot_ref_fail) {
    TEST_BEGIN("dispatch_script_line .cboot 引用失败（非致命）");
    test_setup();
    /* 文件不存在，parser_exec_cboot_ref 返回 -1，但 dispatch 返回 0 */
    char saved_dir[MAX_PATH_LEN];
    strncpy(saved_dir, g_script_dir, sizeof(saved_dir) - 1);
    saved_dir[sizeof(saved_dir) - 1] = '\0';
    strcpy(g_script_dir, "/tmp");
    char *toks[] = {"nonexist/.cboot"};
    int rc = parser_dispatch_script_line(toks, 1);
    ASSERT_EQ_INT(rc, 0, "引用失败应返回 0（非致命）");
    strcpy(g_script_dir, saved_dir);
    TEST_END();
}

TEST_SUITE(test_dispatch_script_line_unknown) {
    TEST_BEGIN("dispatch_script_line 未知命令");
    test_setup();
    char *toks[] = {"foobar"};
    int rc = parser_dispatch_script_line(toks, 1);
    ASSERT_EQ_INT(rc, -1, "未知命令应返回 -1");
    TEST_END();
}

/* ================================================================== */
/* parser_parse_rm (static)                                            */
/* ================================================================== */

TEST_SUITE(test_parse_rm_name_only) {
    TEST_BEGIN("parse_rm 仅名称");
    test_setup();
    char *toks[] = {"rm", "myname"};
    const char *name = NULL;
    int force = -1;
    int rc = parser_parse_rm(toks, 2, &name, &force);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(name, "myname", "name 应为 myname");
    ASSERT_EQ_INT(force, 0, "force 应为 0");
    TEST_END();
}

TEST_SUITE(test_parse_rm_force_before) {
    TEST_BEGIN("parse_rm -f 在前");
    test_setup();
    char *toks[] = {"rm", "-f", "myname"};
    const char *name = NULL;
    int force = -1;
    int rc = parser_parse_rm(toks, 3, &name, &force);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(name, "myname", "name");
    ASSERT_EQ_INT(force, 1, "force 应为 1");
    TEST_END();
}

TEST_SUITE(test_parse_rm_force_after) {
    TEST_BEGIN("parse_rm -f 在后");
    test_setup();
    char *toks[] = {"rm", "myname", "-f"};
    const char *name = NULL;
    int force = -1;
    int rc = parser_parse_rm(toks, 3, &name, &force);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(name, "myname", "name");
    ASSERT_EQ_INT(force, 1, "force 应为 1");
    TEST_END();
}

TEST_SUITE(test_parse_rm_no_name) {
    TEST_BEGIN("parse_rm 无名称");
    test_setup();
    char *toks[] = {"rm"};
    const char *name = NULL;
    int force = -1;
    int rc = parser_parse_rm(toks, 1, &name, &force);
    ASSERT_EQ_INT(rc, -1, "无名称应返回 -1");
    TEST_END();
}

TEST_SUITE(test_parse_rm_only_force) {
    TEST_BEGIN("parse_rm 仅 -f 无名称");
    test_setup();
    char *toks[] = {"rm", "-f"};
    const char *name = NULL;
    int force = -1;
    int rc = parser_parse_rm(toks, 2, &name, &force);
    ASSERT_EQ_INT(rc, -1, "仅有 -f 应返回 -1");
    TEST_END();
}

/* ================================================================== */
/* parser_parse_find_flags (static)                                    */
/* ================================================================== */

TEST_SUITE(test_parse_find_flags_none) {
    TEST_BEGIN("parse_find_flags 无标志");
    test_setup();
    char *toks[] = {"find", "mod", "abc"};
    int flags = parser_parse_find_flags(toks, 3);
    ASSERT_EQ_INT(flags, 0, "无标志应为 0");
    TEST_END();
}

TEST_SUITE(test_parse_find_flags_a) {
    TEST_BEGIN("parse_find_flags -a");
    test_setup();
    char *toks[] = {"find", "mod", "abc", "-a"};
    int flags = parser_parse_find_flags(toks, 4);
    ASSERT_EQ_INT(flags, 1, "-a 应为 1");
    TEST_END();
}

TEST_SUITE(test_parse_find_flags_a5) {
    TEST_BEGIN("parse_find_flags -a5");
    test_setup();
    char *toks[] = {"find", "mod", "abc", "-a5"};
    int flags = parser_parse_find_flags(toks, 4);
    ASSERT_EQ_INT(flags, 5, "-a5 应为 5");
    TEST_END();
}

TEST_SUITE(test_parse_find_flags_multiple) {
    TEST_BEGIN("parse_find_flags 多标志（后者覆盖）");
    test_setup();
    char *toks[] = {"find", "mod", "abc", "-a", "-a3"};
    int flags = parser_parse_find_flags(toks, 5);
    ASSERT_EQ_INT(flags, 3, "后者应覆盖前者");
    TEST_END();
}

TEST_SUITE(test_parse_find_flags_a0) {
    TEST_BEGIN("parse_find_flags -a0");
    test_setup();
    char *toks[] = {"find", "mod", "abc", "-a0"};
    int flags = parser_parse_find_flags(toks, 4);
    ASSERT_EQ_INT(flags, 0, "-a0 应为 0");
    TEST_END();
}

/* ================================================================== */
/* parser_set_script_dir (static)                                      */
/* ================================================================== */

TEST_SUITE(test_set_script_dir_with_slash) {
    TEST_BEGIN("set_script_dir 含路径分隔符");
    test_setup();
    char saved[MAX_PATH_LEN];
    strcpy(g_script_dir, "old_dir");
    parser_set_script_dir("some/path/.cboot", saved, sizeof(saved));
    ASSERT_EQ_STR(saved, "old_dir", "应保存旧目录");
    ASSERT_EQ_STR(g_script_dir, "some/path", "应设为目录部分");
    TEST_END();
}

TEST_SUITE(test_set_script_dir_no_slash) {
    TEST_BEGIN("set_script_dir 无路径分隔符");
    test_setup();
    char saved[MAX_PATH_LEN];
    strcpy(g_script_dir, "old_dir");
    parser_set_script_dir(".cboot", saved, sizeof(saved));
    ASSERT_EQ_STR(saved, "old_dir", "应保存旧目录");
    ASSERT_EQ_STR(g_script_dir, ".", "应设为 .");
    TEST_END();
}

TEST_SUITE(test_set_script_dir_absolute) {
    TEST_BEGIN("set_script_dir 绝对路径");
    test_setup();
    char saved[MAX_PATH_LEN];
    strcpy(g_script_dir, "old");
    parser_set_script_dir("/abs/path/.cboot", saved, sizeof(saved));
    ASSERT_EQ_STR(g_script_dir, "/abs/path", "应设为绝对路径目录");
    TEST_END();
}

/* ================================================================== */
/* parser_read_heredoc (static)                                        */
/* ================================================================== */

TEST_SUITE(test_read_heredoc_normal) {
    TEST_BEGIN("read_heredoc 正常多行");
    test_setup();
    write_temp_file("/tmp/pt_heredoc.txt", "line1\nline2\nEOF\n");
    FILE *f = fopen("/tmp/pt_heredoc.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(line_no, 3, "应读取 3 行");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_NOT_NULL(mod->code, "code 应已设置");
    ASSERT_EQ_STR(mod->code, "line1\nline2", "code 内容应匹配");
    remove("/tmp/pt_heredoc.txt");
    TEST_END();
}

TEST_SUITE(test_read_heredoc_single_line) {
    TEST_BEGIN("read_heredoc 单行");
    test_setup();
    write_temp_file("/tmp/pt_heredoc1.txt", "only line\nEOF\n");
    FILE *f = fopen("/tmp/pt_heredoc1.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_EQ_STR(mod->code, "only line", "code 应为单行");
    remove("/tmp/pt_heredoc1.txt");
    TEST_END();
}

TEST_SUITE(test_read_heredoc_leading_empty) {
    TEST_BEGIN("read_heredoc 前导空行跳过");
    test_setup();
    write_temp_file("/tmp/pt_heredoc2.txt", "\n\nactual\nEOF\n");
    FILE *f = fopen("/tmp/pt_heredoc2.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_EQ_STR(mod->code, "actual", "前导空行应跳过");
    remove("/tmp/pt_heredoc2.txt");
    TEST_END();
}

TEST_SUITE(test_read_heredoc_missing_eof) {
    TEST_BEGIN("read_heredoc 缺少 EOF");
    test_setup();
    write_temp_file("/tmp/pt_heredoc3.txt", "line1\nline2\n");
    FILE *f = fopen("/tmp/pt_heredoc3.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, -1, "缺少 EOF 应返回 -1");
    remove("/tmp/pt_heredoc3.txt");
    TEST_END();
}

TEST_SUITE(test_read_heredoc_empty) {
    TEST_BEGIN("read_heredoc 空内容");
    test_setup();
    write_temp_file("/tmp/pt_heredoc4.txt", "EOF\n");
    FILE *f = fopen("/tmp/pt_heredoc4.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_EQ_STR(mod->code, "", "code 应为空串");
    remove("/tmp/pt_heredoc4.txt");
    TEST_END();
}

TEST_SUITE(test_read_heredoc_trailing_newlines) {
    TEST_BEGIN("read_heredoc 去除尾部换行");
    test_setup();
    /* 内容: "line1\n\n\nEOF\n" -> code 应为 "line1"（尾部空行被去除） */
    write_temp_file("/tmp/pt_heredoc5.txt", "line1\n\n\nEOF\n");
    FILE *f = fopen("/tmp/pt_heredoc5.txt", "r");
    ASSERT_NOT_NULL(f, "文件应打开");
    int line_no = 0;
    int rc = parser_read_heredoc(f, &line_no);
    fclose(f);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_EQ_STR(mod->code, "line1", "尾部空行应被去除");
    remove("/tmp/pt_heredoc5.txt");
    TEST_END();
}

/* ================================================================== */
/* parser_parse_cboot_script (public)                                  */
/* ================================================================== */

TEST_SUITE(test_parse_cboot_script_not_found) {
    TEST_BEGIN("parse_cboot_script 文件不存在");
    test_setup();
    int rc = parser_parse_cboot_script("/tmp/nonexistent_file.cboot");
    ASSERT_EQ_INT(rc, -1, "文件不存在应返回 -1");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_empty) {
    TEST_BEGIN("parse_cboot_script 空文件");
    test_setup();
    write_temp_file("/tmp/pt_empty.cboot", "");
    int rc = parser_parse_cboot_script("/tmp/pt_empty.cboot");
    ASSERT_EQ_INT(rc, 0, "空文件应返回 0");
    remove("/tmp/pt_empty.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_comments) {
    TEST_BEGIN("parse_cboot_script 仅注释");
    test_setup();
    write_temp_file("/tmp/pt_comments.cboot", "# comment 1\n# comment 2\n\n   \n");
    int rc = parser_parse_cboot_script("/tmp/pt_comments.cboot");
    ASSERT_EQ_INT(rc, 0, "仅注释应返回 0");
    remove("/tmp/pt_comments.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_normal) {
    TEST_BEGIN("parse_cboot_script 正常脚本");
    test_setup();
    write_temp_file("/tmp/pt_normal.cboot",
        "# normal script\n"
        "mod mymod\n"
        "cd mymod\n"
        "cmt \"test module\"\n"
        "cd ..\n");
    int rc = parser_parse_cboot_script("/tmp/pt_normal.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    /* 验证模块被创建（commands_cmd_mod 做真实工作） */
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "mymod"), "模块应创建");
    remove("/tmp/pt_normal.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_project_cmd) {
    TEST_BEGIN("parse_cboot_script project 命令");
    test_setup();
    write_temp_file("/tmp/pt_project.cboot", "project myproject\n");
    int rc = parser_parse_cboot_script("/tmp/pt_project.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_proj->name, "myproject", "项目名应更新");
    remove("/tmp/pt_project.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_heredoc) {
    TEST_BEGIN("parse_cboot_script heredoc 代码块");
    test_setup();
    write_temp_file("/tmp/pt_heredoc_script.cboot",
        "mod hmod\n"
        "cd hmod\n"
        "code <<EOF\n"
        "int x = 42;\n"
        "return x;\n"
        "EOF\n"
        "cd ..\n");
    int rc = parser_parse_cboot_script("/tmp/pt_heredoc_script.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    Domain *hmod = domain_domain_find_child(g_proj->root, "hmod");
    ASSERT_NOT_NULL(hmod, "模块 hmod 应创建");
    ModuleDomain *mod = (ModuleDomain *)hmod;
    ASSERT_NOT_NULL(mod->code, "code 应已设置");
    ASSERT_EQ_STR(mod->code, "int x = 42;\nreturn x;", "code 内容应匹配");
    remove("/tmp/pt_heredoc_script.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_update) {
    TEST_BEGIN("parse_cboot_script update 提前返回");
    test_setup();
    write_temp_file("/tmp/pt_update.cboot",
        "mod before_update\n"
        "update\n"
        "mod after_update\n");  /* 此行不应执行 */
    int rc = parser_parse_cboot_script("/tmp/pt_update.cboot");
    ASSERT_EQ_INT(rc, 0, "update 后应返回 0");
    /* update 之前的命令应执行 */
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "before_update"),
                    "update 前的模块应创建");
    /* update 之后的命令不应执行 */
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "after_update"),
                "update 后的模块不应创建");
    remove("/tmp/pt_update.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_error) {
    TEST_BEGIN("parse_cboot_script 错误命令");
    test_setup();
    write_temp_file("/tmp/pt_error.cboot",
        "mod good_mod\n"
        "unknown_command arg1\n"  /* 未知命令，返回 -1 */
        "mod bad_mod\n");  /* 此行不应执行 */
    int rc = parser_parse_cboot_script("/tmp/pt_error.cboot");
    ASSERT_EQ_INT(rc, -1, "未知命令应导致返回 -1");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "good_mod"),
                    "错误前的模块应创建");
    ASSERT_NULL(domain_domain_find_child(g_proj->root, "bad_mod"),
                "错误后的模块不应创建");
    remove("/tmp/pt_error.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_code_inline) {
    TEST_BEGIN("parse_cboot_script 内联 code 命令");
    test_setup();
    write_temp_file("/tmp/pt_code_inline.cboot",
        "mod cmod\n"
        "cd cmod\n"
        "code return 42;\n"
        "cd ..\n");
    int rc = parser_parse_cboot_script("/tmp/pt_code_inline.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    Domain *cmod = domain_domain_find_child(g_proj->root, "cmod");
    ASSERT_NOT_NULL(cmod, "模块应创建");
    ModuleDomain *mod = (ModuleDomain *)cmod;
    ASSERT_EQ_STR(mod->code, "return 42;", "code 应设置");
    remove("/tmp/pt_code_inline.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_blank_lines) {
    TEST_BEGIN("parse_cboot_script 空行和空白行");
    test_setup();
    write_temp_file("/tmp/pt_blanks.cboot",
        "\n"
        "   \n"
        "mod blankmod\n"
        "\n"
        "   \n");
    int rc = parser_parse_cboot_script("/tmp/pt_blanks.cboot");
    ASSERT_EQ_INT(rc, 0, "空行应跳过");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "blankmod"),
                    "模块应创建");
    remove("/tmp/pt_blanks.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_exit) {
    TEST_BEGIN("parse_cboot_script exit 命令");
    test_setup();
    g_running = 1;
    write_temp_file("/tmp/pt_exit.cboot",
        "mod before_exit\n"
        "exit\n"
        "mod after_exit\n");  /* exit 不导致脚本终止，只是设 g_running=0 */
    int rc = parser_parse_cboot_script("/tmp/pt_exit.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(g_running, 0, "g_running 应为 0");
    ASSERT_NOT_NULL(domain_domain_find_child(g_proj->root, "after_exit"),
                    "exit 后的命令仍应执行（exit 仅设标志）");
    remove("/tmp/pt_exit.cboot");
    TEST_END();
}

TEST_SUITE(test_parse_cboot_script_restores_script_dir) {
    TEST_BEGIN("parse_cboot_script 恢复 g_script_dir");
    test_setup();
    strcpy(g_script_dir, "before_parse");
    write_temp_file("/tmp/pt_restore.cboot", "# comment only\n");
    int rc = parser_parse_cboot_script("/tmp/pt_restore.cboot");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_STR(g_script_dir, "before_parse", "g_script_dir 应恢复");
    remove("/tmp/pt_restore.cboot");
    TEST_END();
}

/* ================================================================== */
/* 主函数                                                              */
/* ================================================================== */
int main(void) {
    printf("========== parser/parser.c 测试 ==========\n");

    /* parser_is_cboot_ref */
    RUN_SUITE(test_is_cboot_ref_null);
    RUN_SUITE(test_is_cboot_ref_exact);
    RUN_SUITE(test_is_cboot_ref_single_level);
    RUN_SUITE(test_is_cboot_ref_multi_level);
    RUN_SUITE(test_is_cboot_ref_not_ref);
    RUN_SUITE(test_is_cboot_ref_too_short);
    RUN_SUITE(test_is_cboot_ref_long_ref);

    /* parser_try_cboot_ref */
    RUN_SUITE(test_try_cboot_ref_not_ref);
    RUN_SUITE(test_try_cboot_ref_is_ref);
    RUN_SUITE(test_try_cboot_ref_null);

    /* parser_exec_cboot_ref */
    RUN_SUITE(test_exec_cboot_ref_null);
    RUN_SUITE(test_exec_cboot_ref_no_slash);
    RUN_SUITE(test_exec_cboot_ref_single_level);
    RUN_SUITE(test_exec_cboot_ref_multi_level);
    RUN_SUITE(test_exec_cboot_ref_mod_fail);
    RUN_SUITE(test_exec_cboot_ref_file_not_found);

    /* parser_join_tokens_from */
    RUN_SUITE(test_join_tokens_empty);
    RUN_SUITE(test_join_tokens_single);
    RUN_SUITE(test_join_tokens_multiple);
    RUN_SUITE(test_join_tokens_from_zero);

    /* parser_require_args */
    RUN_SUITE(test_require_args_sufficient);
    RUN_SUITE(test_require_args_insufficient);
    RUN_SUITE(test_require_args_exact);

    /* parser_dispatch_build */
    RUN_SUITE(test_dispatch_build_project);
    RUN_SUITE(test_dispatch_build_project_no_args);
    RUN_SUITE(test_dispatch_build_mod);
    RUN_SUITE(test_dispatch_build_mod_no_args);
    RUN_SUITE(test_dispatch_build_struct);
    RUN_SUITE(test_dispatch_build_type);
    RUN_SUITE(test_dispatch_build_def);
    RUN_SUITE(test_dispatch_build_void);
    RUN_SUITE(test_dispatch_build_void_multi_type);
    RUN_SUITE(test_dispatch_build_void_no_args);
    RUN_SUITE(test_dispatch_build_var);
    RUN_SUITE(test_dispatch_build_mem);
    RUN_SUITE(test_dispatch_build_enum);
    RUN_SUITE(test_dispatch_build_enum_no_args);
    RUN_SUITE(test_dispatch_build_unknown);
    RUN_SUITE(test_dispatch_build_mod_fail);

    /* parser_dispatch_modify */
    RUN_SUITE(test_dispatch_modify_value);
    RUN_SUITE(test_dispatch_modify_value_no_args);
    RUN_SUITE(test_dispatch_modify_cmode);
    RUN_SUITE(test_dispatch_modify_cmt);
    RUN_SUITE(test_dispatch_modify_cmt_no_args);
    RUN_SUITE(test_dispatch_modify_call);
    RUN_SUITE(test_dispatch_modify_mode);
    RUN_SUITE(test_dispatch_modify_code_inline);
    RUN_SUITE(test_dispatch_modify_code_heredoc_signal);
    RUN_SUITE(test_dispatch_modify_code_alone);
    RUN_SUITE(test_dispatch_modify_unknown);

    /* parser_dispatch_control */
    RUN_SUITE(test_dispatch_control_cd_with_path);
    RUN_SUITE(test_dispatch_control_cd_no_path);
    RUN_SUITE(test_dispatch_control_ls_with_name);
    RUN_SUITE(test_dispatch_control_ls_no_name);
    RUN_SUITE(test_dispatch_control_exit);
    RUN_SUITE(test_dispatch_control_mv);
    RUN_SUITE(test_dispatch_control_mv_no_args);
    RUN_SUITE(test_dispatch_control_rm);
    RUN_SUITE(test_dispatch_control_rm_force);
    RUN_SUITE(test_dispatch_control_rm_no_name);
    RUN_SUITE(test_dispatch_control_find);
    RUN_SUITE(test_dispatch_control_find_with_flags);
    RUN_SUITE(test_dispatch_control_find_no_args);
    RUN_SUITE(test_dispatch_control_unknown);

    /* parser_dispatch_action */
    RUN_SUITE(test_dispatch_action_gen);
    RUN_SUITE(test_dispatch_action_gen_skip);
    RUN_SUITE(test_dispatch_action_analyze);
    RUN_SUITE(test_dispatch_action_update);
    RUN_SUITE(test_dispatch_action_im);
    RUN_SUITE(test_dispatch_action_im_no_args);
    RUN_SUITE(test_dispatch_action_in);
    RUN_SUITE(test_dispatch_action_in_no_args);
    RUN_SUITE(test_dispatch_action_res);
    RUN_SUITE(test_dispatch_action_res_no_args);
    RUN_SUITE(test_dispatch_action_unknown);

    /* parser_dispatch_script_line */
    RUN_SUITE(test_dispatch_script_line_build);
    RUN_SUITE(test_dispatch_script_line_action);
    RUN_SUITE(test_dispatch_script_line_control);
    RUN_SUITE(test_dispatch_script_line_cboot_ref);
    RUN_SUITE(test_dispatch_script_line_cboot_ref_fail);
    RUN_SUITE(test_dispatch_script_line_unknown);

    /* parser_parse_rm */
    RUN_SUITE(test_parse_rm_name_only);
    RUN_SUITE(test_parse_rm_force_before);
    RUN_SUITE(test_parse_rm_force_after);
    RUN_SUITE(test_parse_rm_no_name);
    RUN_SUITE(test_parse_rm_only_force);

    /* parser_parse_find_flags */
    RUN_SUITE(test_parse_find_flags_none);
    RUN_SUITE(test_parse_find_flags_a);
    RUN_SUITE(test_parse_find_flags_a5);
    RUN_SUITE(test_parse_find_flags_multiple);
    RUN_SUITE(test_parse_find_flags_a0);

    /* parser_set_script_dir */
    RUN_SUITE(test_set_script_dir_with_slash);
    RUN_SUITE(test_set_script_dir_no_slash);
    RUN_SUITE(test_set_script_dir_absolute);

    /* parser_read_heredoc */
    RUN_SUITE(test_read_heredoc_normal);
    RUN_SUITE(test_read_heredoc_single_line);
    RUN_SUITE(test_read_heredoc_leading_empty);
    RUN_SUITE(test_read_heredoc_missing_eof);
    RUN_SUITE(test_read_heredoc_empty);
    RUN_SUITE(test_read_heredoc_trailing_newlines);

    /* parser_parse_cboot_script */
    RUN_SUITE(test_parse_cboot_script_not_found);
    RUN_SUITE(test_parse_cboot_script_empty);
    RUN_SUITE(test_parse_cboot_script_comments);
    RUN_SUITE(test_parse_cboot_script_normal);
    RUN_SUITE(test_parse_cboot_script_project_cmd);
    RUN_SUITE(test_parse_cboot_script_heredoc);
    RUN_SUITE(test_parse_cboot_script_update);
    RUN_SUITE(test_parse_cboot_script_error);
    RUN_SUITE(test_parse_cboot_script_code_inline);
    RUN_SUITE(test_parse_cboot_script_blank_lines);
    RUN_SUITE(test_parse_cboot_script_exit);
    RUN_SUITE(test_parse_cboot_script_restores_script_dir);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
