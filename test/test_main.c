/*
 * test_main.c - main/main.c 单元测试
 *
 * 通过 #include "main/main.c" 方式包含被测源文件以测试 static 函数。
 * main.c 依赖: utils, domain/core, domain, typecheck, cupdate, parser(mock), commands(mock)
 *
 * main() 函数通过 #define main cboot_main 重命名以避免与测试 main() 冲突。
 *
 * 测试覆盖：
 *   - 命令行参数解析 (main_parse_flag, main_parse_info_flag, main_parse_positional,
 *     main_parse_args)
 *   - 运行模式入口 (main_run_update, main_run_adjust, main_run_analyze,
 *     main_run_batch_script, main_run_default_cboot, main_run_interactive)
 *   - main 主入口 (cboot_main)
 *   - 辅助函数 (main_detect_fine_tune_mode, main_print_usage, main_join_rest)
 *   - Tab 补全 (main_try_complete, main_common_prefix_len, tab_collect_child_domains,
 *     tab_base_len, tab_apply_single, tab_apply_multi, tab_collect_for_cmd,
 *     main_do_tab_complete)
 *   - REPL 辅助 (main_repl_history_up/down, main_repl_handle_esc/handle_esc2/handle_ctrl,
 *     main_repl_read_line, main_repl_loop)
 *   - 命令分发 (main_dispatch_build/modify/control/action/command)
 *   - 代码读取 (main_read_code_from_stdin)
 *   - 参数解析 (main_parse_rm, main_parse_find_flags)
 */
#include "test.h"

#include <unistd.h>
#include <fcntl.h>

/* Include dependencies (order matters: dependencies first) */
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "cupdate/cupdate.c"
#include "cupdate/cupdate_lexer.c"
#include "cupdate/cupdate_parser.c"

/* ------------------------------------------------------------------ */
/* Global state                                                        */
/* g_script_dir declared extern in cboot.h; defined here since         */
/* parser.c is NOT included (parser functions are mocked).             */
/* g_proj/g_mode/g_force/g_running/g_skip_gen defined in main.c below. */
/* ------------------------------------------------------------------ */
char g_script_dir[MAX_PATH_LEN] = ".";

/* ------------------------------------------------------------------ */
/* Mock: generator / docgen (cupdate.c calls generator_generate_cboot_only) */
/* ------------------------------------------------------------------ */
int generator_generate_cboot_only(Project *proj) { (void)proj; return 0; }
int generator_generate_project(Project *proj)    { (void)proj; return 0; }
int docgen_generate_docs(Project *proj, const char *dir) { (void)proj; (void)dir; return 0; }

/* ------------------------------------------------------------------ */
/* Mock: parser functions                                              */
/* ------------------------------------------------------------------ */
static int g_mock_parser_return = 0;
static int g_mock_parser_calls = 0;
static const char *g_mock_parser_last_file = NULL;
int parser_parse_cboot_script(const char *filename) {
    g_mock_parser_calls++;
    g_mock_parser_last_file = filename;
    return g_mock_parser_return;
}

static int g_mock_try_ref_return = 0;
static int g_mock_try_ref_calls = 0;
static const char *g_mock_try_ref_last = NULL;
int parser_try_cboot_ref(const char *token) {
    g_mock_try_ref_calls++;
    g_mock_try_ref_last = token;
    return g_mock_try_ref_return;
}

/* ------------------------------------------------------------------ */
/* Mock: commands_cmd_* functions                                      */
/* ------------------------------------------------------------------ */
static const char *g_mock_last_cmd = "";
static const char *g_mock_last_arg1 = NULL;
static const char *g_mock_last_arg2 = NULL;
static int g_mock_last_int = 0;
static int g_mock_call_count = 0;
static int g_mock_fail = 0;
static int g_mock_exit_called = 0;

static void mock_reset(void) {
    g_mock_last_cmd = "";
    g_mock_last_arg1 = NULL;
    g_mock_last_arg2 = NULL;
    g_mock_last_int = 0;
    g_mock_call_count = 0;
    g_mock_fail = 0;
    g_mock_exit_called = 0;
    g_mock_parser_return = 0;
    g_mock_parser_calls = 0;
    g_mock_parser_last_file = NULL;
    g_mock_try_ref_return = 0;
    g_mock_try_ref_calls = 0;
    g_mock_try_ref_last = NULL;
}

int commands_cmd_mod(const char *name) {
    g_mock_last_cmd = "mod"; g_mock_last_arg1 = name; g_mock_call_count++;
    if (g_mock_fail) { g_mock_fail = 0; return -1; }
    return 0;
}
int commands_cmd_struct(const char *name) { g_mock_last_cmd = "struct"; g_mock_last_arg1 = name; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_type(const char *name) { g_mock_last_cmd = "type"; g_mock_last_arg1 = name; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_def(const char *name) { g_mock_last_cmd = "def"; g_mock_last_arg1 = name; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_void(const char *name, const char *rt) { g_mock_last_cmd = "void"; g_mock_last_arg1 = name; g_mock_last_arg2 = rt; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_var(const char *name, const char *t) { g_mock_last_cmd = "var"; g_mock_last_arg1 = name; g_mock_last_arg2 = t; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_mem(const char *name, const char *t) { g_mock_last_cmd = "mem"; g_mock_last_arg1 = name; g_mock_last_arg2 = t; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_enum(const char *defs, const char *start) { g_mock_last_cmd = "enum"; g_mock_last_arg1 = defs; g_mock_last_arg2 = start; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_cmt(const char *text) { g_mock_last_cmd = "cmt"; g_mock_last_arg1 = text; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_value(const char *text) { g_mock_last_cmd = "value"; g_mock_last_arg1 = text; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_call(const char *cc) { g_mock_last_cmd = "call"; g_mock_last_arg1 = cc; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_mode(const char *text) { g_mock_last_cmd = "mode"; g_mock_last_arg1 = text; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_cmode(const char *text) { g_mock_last_cmd = "cmode"; g_mock_last_arg1 = text; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_cd(const char *path) { g_mock_last_cmd = "cd"; g_mock_last_arg1 = path; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_ls(const char *name) { g_mock_last_cmd = "ls"; g_mock_last_arg1 = name; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_mv(const char *src, const char *tgt) { g_mock_last_cmd = "mv"; g_mock_last_arg1 = src; g_mock_last_arg2 = tgt; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_rm(const char *name, int force) { g_mock_last_cmd = "rm"; g_mock_last_arg1 = name; g_mock_last_int = force; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_find(const char *tf, const char *pat, int flags) { g_mock_last_cmd = "find"; g_mock_last_arg1 = tf; g_mock_last_arg2 = pat; g_mock_last_int = flags; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_gen(void) { g_mock_last_cmd = "gen"; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_update(void) { g_mock_last_cmd = "update"; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_analyze(void) { g_mock_last_cmd = "analyze"; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_adjust(void) { g_mock_last_cmd = "adjust"; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_im(const char *path) { g_mock_last_cmd = "im"; g_mock_last_arg1 = path; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_in(const char *path) { g_mock_last_cmd = "in"; g_mock_last_arg1 = path; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_res(const char *fp) { g_mock_last_cmd = "res"; g_mock_last_arg1 = fp; g_mock_call_count++; if (g_mock_fail) { g_mock_fail = 0; return -1; } return 0; }
int commands_cmd_exit(void) { g_mock_last_cmd = "exit"; g_mock_call_count++; g_mock_exit_called = 1; g_running = 0; return 0; }

/* ------------------------------------------------------------------ */
/* Include the file under test (main renamed to avoid conflict)        */
/* ------------------------------------------------------------------ */
#define main cboot_main
#include "main/main.c"
#undef main

/* ------------------------------------------------------------------ */
/* Test helpers                                                        */
/* ------------------------------------------------------------------ */
#define CASE(name)   TEST_BEGIN(name); {
#define ENDCASE      TEST_END(); }

static char g_orig_cwd[MAX_PATH_LEN];
static void save_cwd(void) { getcwd(g_orig_cwd, sizeof(g_orig_cwd)); }
static void restore_cwd(void) { chdir(g_orig_cwd); }

static void reset_state(void) {
    if (g_proj) { domain_project_free(g_proj); g_proj = NULL; }
    g_mode = MODE_INTERACTIVE;
    g_force = 0;
    g_running = 1;
    g_skip_gen = 0;
    mock_reset();
}

static void reset_proj(void) {
    if (g_proj) domain_project_free(g_proj);
    g_proj = domain_project_new("test");
}

static int g_saved_stdout = -1;
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

static int g_saved_stderr = -1;
static void suppress_stderr(void) {
    fflush(stderr);
    g_saved_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
}
static void restore_stderr(void) {
    if (g_saved_stderr >= 0) {
        fflush(stderr);
        dup2(g_saved_stderr, STDERR_FILENO);
        close(g_saved_stderr);
        g_saved_stderr = -1;
    }
}

static int g_saved_stdin = -1;
static void feed_stdin(const char *s) {
    g_saved_stdin = dup(STDIN_FILENO);
    FILE *f = tmpfile();
    if (!f) return;
    fputs(s, f);
    fflush(f);
    rewind(f);
    dup2(fileno(f), STDIN_FILENO);
    fclose(f);
    clearerr(stdin);
}
static void restore_stdin(void) {
    if (g_saved_stdin >= 0) {
        dup2(g_saved_stdin, STDIN_FILENO);
        close(g_saved_stdin);
        g_saved_stdin = -1;
        clearerr(stdin);
    }
}

static int write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

static void make_tmp_dir(const char *path) {
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'", path, path);
    system(cmd);
}

/* 创建 tokens 数组（静态内存，测试内使用） */
static char **make_tokens_from_strings(const char **strs, int n) {
    static char *tokens_buf[64];
    static char token_storage[64][MAX_LINE_LEN];
    for (int i = 0; i < n && i < 64; i++) {
        strncpy(token_storage[i], strs[i], MAX_LINE_LEN - 1);
        token_storage[i][MAX_LINE_LEN - 1] = '\0';
        tokens_buf[i] = token_storage[i];
    }
    return tokens_buf;
}

/* ================================================================== */
/* 1. main_parse_flag                                                  */
/* ================================================================== */
TEST_SUITE(test_parse_flag) {
    int adjust, analyze, update;

    CASE("-f 设置 g_force=1 返回 1");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("-f", &adjust, &analyze, &update), 1, "-f 返回 1");
    ASSERT_EQ_INT(g_force, 1, "g_force 应为 1");
    ASSERT_EQ_INT(update, 0, "update 未变");
    ENDCASE;

    CASE("--force 设置 g_force=1 返回 1");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("--force", &adjust, &analyze, &update), 1, "--force 返回 1");
    ASSERT_EQ_INT(g_force, 1, "g_force 应为 1");
    ENDCASE;

    CASE("-update 设置 update_mode=1");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("-update", &adjust, &analyze, &update), 1, "-update 返回 1");
    ASSERT_EQ_INT(update, 1, "update_mode 应为 1");
    ASSERT_EQ_INT(g_force, 0, "g_force 未变");
    ENDCASE;

    CASE("adjust 设置 adjust_mode=1");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("adjust", &adjust, &analyze, &update), 1, "adjust 返回 1");
    ASSERT_EQ_INT(adjust, 1, "adjust_mode 应为 1");
    ENDCASE;

    CASE("analyze 设置 analyze_mode=1");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("analyze", &adjust, &analyze, &update), 1, "analyze 返回 1");
    ASSERT_EQ_INT(analyze, 1, "analyze_mode 应为 1");
    ENDCASE;

    CASE("未知参数返回 0");
    reset_state();
    adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_flag("-x", &adjust, &analyze, &update), 0, "未知参数返回 0");
    ASSERT_EQ_INT(main_parse_flag("foo", &adjust, &analyze, &update), 0, "foo 返回 0");
    ASSERT_EQ_INT(main_parse_flag("-h", &adjust, &analyze, &update), 0, "-h 不在 flag 中");
    ENDCASE;
}

/* ================================================================== */
/* 2. main_parse_info_flag                                             */
/* ================================================================== */
TEST_SUITE(test_parse_info_flag) {
    CASE("-h 返回 1 (打印 usage)");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_info_flag("-h", "cboot"), 1, "-h 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("--help 返回 1");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_info_flag("--help", "cboot"), 1, "--help 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("-v 返回 1 (打印版本)");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_info_flag("-v", "cboot"), 1, "-v 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("--version 返回 1");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_info_flag("--version", "cboot"), 1, "--version 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("非 info flag 返回 0");
    reset_state();
    ASSERT_EQ_INT(main_parse_info_flag("-f", "cboot"), 0, "-f 不是 info flag");
    ASSERT_EQ_INT(main_parse_info_flag("foo", "cboot"), 0, "foo 返回 0");
    ASSERT_EQ_INT(main_parse_info_flag("-update", "cboot"), 0, "-update 返回 0");
    ENDCASE;
}

/* ================================================================== */
/* 3. main_parse_positional                                            */
/* ================================================================== */
TEST_SUITE(test_parse_positional) {
    const char *proj, *script;
    CASE(".cboot 后缀 → script_file");
    proj = NULL; script = NULL;
    main_parse_positional("foo.cboot", &proj, &script);
    ASSERT_EQ_STR(script, "foo.cboot", "script_file 应为 foo.cboot");
    ASSERT_NULL(proj, "proj_name 应为 NULL");
    ENDCASE;

    CASE("纯 .cboot → script_file");
    proj = NULL; script = NULL;
    main_parse_positional(".cboot", &proj, &script);
    ASSERT_EQ_STR(script, ".cboot", "script_file 应为 .cboot");
    ASSERT_NULL(proj, "proj_name 应为 NULL");
    ENDCASE;

    CASE("多级路径 .cboot → script_file");
    proj = NULL; script = NULL;
    main_parse_positional("a/b/.cboot", &proj, &script);
    ASSERT_EQ_STR(script, "a/b/.cboot", "script_file 应为 a/b/.cboot");
    ENDCASE;

    CASE("普通名称 → proj_name");
    proj = NULL; script = NULL;
    main_parse_positional("myproject", &proj, &script);
    ASSERT_EQ_STR(proj, "myproject", "proj_name 应为 myproject");
    ASSERT_NULL(script, "script_file 应为 NULL");
    ENDCASE;

    CASE("短名称 → proj_name");
    proj = NULL; script = NULL;
    main_parse_positional("ab", &proj, &script);
    ASSERT_EQ_STR(proj, "ab", "proj_name 应为 ab");
    ENDCASE;

    CASE("不以 .cboot 结尾的长名 → proj_name");
    proj = NULL; script = NULL;
    main_parse_positional("a.cboo", &proj, &script);
    ASSERT_EQ_STR(proj, "a.cboo", "a.cboo 不是 .cboot 后缀");
    ENDCASE;
}

/* ================================================================== */
/* 4. main_parse_args                                                  */
/* ================================================================== */
TEST_SUITE(test_parse_args) {
    const char *proj, *script;
    int adjust, analyze, update;
    char *argv_noargs[] = {"cboot"};
    char *argv_help[] = {"cboot", "--help"};
    char *argv_ver[] = {"cboot", "-v"};
    char *argv_force[] = {"cboot", "-f", "myproj"};
    char *argv_script[] = {"cboot", "run.cboot"};
    char *argv_unknown[] = {"cboot", "-x"};
    char *argv_analyze[] = {"cboot", "analyze"};
    char *argv_adjust[] = {"cboot", "adjust"};
    char *argv_update[] = {"cboot", "-update"};

    CASE("无参数返回 0, 全部为空/0");
    reset_state();
    proj = script = NULL; adjust = analyze = update = 0;
    ASSERT_EQ_INT(main_parse_args(1, argv_noargs, &proj, &script, &adjust, &analyze, &update), 0, "无参数返回 0");
    ASSERT_NULL(proj, "proj 为 NULL");
    ASSERT_NULL(script, "script 为 NULL");
    ENDCASE;

    CASE("--help 返回 1");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_args(2, argv_help, &proj, &script, &adjust, &analyze, &update), 1, "--help 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("-v 返回 1");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(main_parse_args(2, argv_ver, &proj, &script, &adjust, &analyze, &update), 1, "-v 返回 1");
    restore_stdout();
    ENDCASE;

    CASE("-f myproj → g_force=1, proj_name=myproj");
    reset_state();
    proj = script = NULL;
    ASSERT_EQ_INT(main_parse_args(3, argv_force, &proj, &script, &adjust, &analyze, &update), 0, "返回 0");
    ASSERT_EQ_INT(g_force, 1, "g_force=1");
    ASSERT_EQ_STR(proj, "myproj", "proj_name=myproj");
    ENDCASE;

    CASE("run.cboot → script_file");
    reset_state();
    proj = script = NULL;
    ASSERT_EQ_INT(main_parse_args(2, argv_script, &proj, &script, &adjust, &analyze, &update), 0, "返回 0");
    ASSERT_EQ_STR(script, "run.cboot", "script_file=run.cboot");
    ENDCASE;

    CASE("未知选项 -x 返回 -1");
    reset_state();
    suppress_stdout(); suppress_stderr();
    ASSERT_EQ_INT(main_parse_args(2, argv_unknown, &proj, &script, &adjust, &analyze, &update), -1, "-x 返回 -1");
    restore_stderr(); restore_stdout();
    ENDCASE;

    CASE("analyze → analyze_mode=1");
    reset_state();
    analyze = 0;
    ASSERT_EQ_INT(main_parse_args(2, argv_analyze, &proj, &script, &adjust, &analyze, &update), 0, "返回 0");
    ASSERT_EQ_INT(analyze, 1, "analyze_mode=1");
    ENDCASE;

    CASE("adjust → adjust_mode=1");
    reset_state();
    adjust = 0;
    ASSERT_EQ_INT(main_parse_args(2, argv_adjust, &proj, &script, &adjust, &analyze, &update), 0, "返回 0");
    ASSERT_EQ_INT(adjust, 1, "adjust_mode=1");
    ENDCASE;

    CASE("-update → update_mode=1");
    reset_state();
    update = 0;
    ASSERT_EQ_INT(main_parse_args(2, argv_update, &proj, &script, &adjust, &analyze, &update), 0, "返回 0");
    ASSERT_EQ_INT(update, 1, "update_mode=1");
    ENDCASE;
}

/* ================================================================== */
/* 5. main_run_update / adjust / analyze / batch / default             */
/* ================================================================== */
TEST_SUITE(test_run_modes) {
    CASE("main_run_update: 无 .cboot 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_update");
    chdir("/tmp/cboot_test_run_update");
    suppress_stdout(); suppress_stderr();
    int rc = main_run_update();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc, 1, "无 .cboot 应返回 1");
    ASSERT_NULL(g_proj, "g_proj 应为 NULL");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_update");
    restore_cwd();
    ENDCASE;

    CASE("main_run_update: parser 失败返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_update2");
    chdir("/tmp/cboot_test_run_update2");
    write_temp_file(".cboot", "");
    g_mock_parser_return = -1;
    suppress_stdout(); suppress_stderr();
    int rc2 = main_run_update();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc2, 1, "parser 失败应返回 1");
    ASSERT_EQ_INT(g_mock_parser_calls, 1, "parser 应被调用 1 次");
    ASSERT_EQ_INT(g_skip_gen, 0, "g_skip_gen 应恢复为 0");
    if (g_proj) { domain_project_free(g_proj); g_proj = NULL; }
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_update2");
    restore_cwd();
    ENDCASE;

    CASE("main_run_update: 成功返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_update3");
    chdir("/tmp/cboot_test_run_update3");
    write_temp_file(".cboot", "");
    g_mock_parser_return = 0;
    suppress_stdout(); suppress_stderr();
    int rc3 = main_run_update();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc3, 0, "成功应返回 0");
    ASSERT_EQ_INT(g_mock_parser_calls, 1, "parser 被调用");
    ASSERT_EQ_INT(g_mock_call_count, 2, "update + analyze = 2 次调用");
    ASSERT_EQ_STR(g_mock_last_cmd, "analyze", "最后调用 analyze");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_update3");
    restore_cwd();
    ENDCASE;

    CASE("main_run_adjust: 无 .cboot 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_adjust");
    chdir("/tmp/cboot_test_run_adjust");
    suppress_stdout(); suppress_stderr();
    int rc4 = main_run_adjust();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc4, 1, "无 .cboot 返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_adjust");
    restore_cwd();
    ENDCASE;

    CASE("main_run_adjust: 成功 (g_running=0 跳过 repl)");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_adjust2");
    chdir("/tmp/cboot_test_run_adjust2");
    write_temp_file(".cboot", "");
    g_mock_parser_return = 0;
    g_running = 0;  /* 跳过 repl_loop */
    suppress_stdout(); suppress_stderr();
    int rc5 = main_run_adjust();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc5, 0, "成功返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "adjust", "应调用 adjust");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_adjust2");
    restore_cwd();
    ENDCASE;

    CASE("main_run_analyze: 无 .cboot 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_analyze");
    chdir("/tmp/cboot_test_run_analyze");
    suppress_stdout(); suppress_stderr();
    int rc6 = main_run_analyze();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc6, 1, "无 .cboot 返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_analyze");
    restore_cwd();
    ENDCASE;

    CASE("main_run_analyze: 成功返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_run_analyze2");
    chdir("/tmp/cboot_test_run_analyze2");
    write_temp_file(".cboot", "");
    g_mock_parser_return = 0;
    suppress_stdout(); suppress_stderr();
    int rc7 = main_run_analyze();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc7, 0, "成功返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "analyze", "应调用 analyze");
    ASSERT_EQ_INT(g_skip_gen, 0, "g_skip_gen 恢复 0");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_run_analyze2");
    restore_cwd();
    ENDCASE;

    CASE("main_run_batch_script: 文件不存在返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_batch");
    chdir("/tmp/cboot_test_batch");
    suppress_stdout(); suppress_stderr();
    int rc8 = main_run_batch_script("nonexist.cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc8, 1, "文件不存在返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_batch");
    restore_cwd();
    ENDCASE;

    CASE("main_run_batch_script: parser 失败返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_batch2");
    chdir("/tmp/cboot_test_batch2");
    write_temp_file("test.cboot", "");
    g_mock_parser_return = -1;
    suppress_stdout(); suppress_stderr();
    int rc9 = main_run_batch_script("test.cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc9, 1, "parser 失败返回 1");
    if (g_proj) { domain_project_free(g_proj); g_proj = NULL; }
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_batch2");
    restore_cwd();
    ENDCASE;

    CASE("main_run_batch_script: 成功返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_batch3");
    chdir("/tmp/cboot_test_batch3");
    write_temp_file("ok.cboot", "");
    g_mock_parser_return = 0;
    suppress_stdout(); suppress_stderr();
    int rc10 = main_run_batch_script("ok.cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc10, 0, "成功返回 0");
    ASSERT_EQ_INT(g_mock_parser_calls, 1, "parser 被调用");
    ASSERT_EQ_STR(g_mock_parser_last_file, "ok.cboot", "传入正确文件名");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_batch3");
    restore_cwd();
    ENDCASE;

    CASE("main_run_default_cboot: 无 .cboot 返回 -1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_default");
    chdir("/tmp/cboot_test_default");
    int rc11 = main_run_default_cboot();
    ASSERT_EQ_INT(rc11, -1, "无 .cboot 返回 -1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_default");
    restore_cwd();
    ENDCASE;

    CASE("main_run_default_cboot: 成功返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_default2");
    chdir("/tmp/cboot_test_default2");
    write_temp_file(".cboot", "");
    g_mock_parser_return = 0;
    suppress_stdout(); suppress_stderr();
    int rc12 = main_run_default_cboot();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc12, 0, "成功返回 0");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_default2");
    restore_cwd();
    ENDCASE;

    CASE("main_run_default_cboot: parser 失败返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_default3");
    chdir("/tmp/cboot_test_default3");
    write_temp_file(".cboot", "");
    g_mock_parser_return = -1;
    suppress_stdout(); suppress_stderr();
    int rc13 = main_run_default_cboot();
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc13, 1, "parser 失败返回 1");
    if (g_proj) { domain_project_free(g_proj); g_proj = NULL; }
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_default3");
    restore_cwd();
    ENDCASE;
}

/* ================================================================== */
/* 6. main_run_interactive                                             */
/* ================================================================== */
TEST_SUITE(test_run_interactive) {
    CASE("无效项目名返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_interactive");
    chdir("/tmp/cboot_test_interactive");
    suppress_stdout(); suppress_stderr();
    int rc = main_run_interactive("123bad", "cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc, 1, "无效名返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_interactive");
    restore_cwd();
    ENDCASE;

    CASE("目录已存在且无 -f 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_interactive2");
    chdir("/tmp/cboot_test_interactive2");
    make_tmp_dir("existing_dir");
    g_force = 0;
    suppress_stdout(); suppress_stderr();
    int rc2 = main_run_interactive("existing_dir", "cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc2, 1, "目录已存在返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_interactive2");
    restore_cwd();
    ENDCASE;

    CASE("有效名 + g_running=0 → 创建目录返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_interactive3");
    chdir("/tmp/cboot_test_interactive3");
    g_running = 0;
    suppress_stdout(); suppress_stderr();
    int rc3 = main_run_interactive("newproj", "cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc3, 0, "有效名返回 0");
    ASSERT_EQ_INT(g_mode, MODE_INTERACTIVE, "mode=INTERACTIVE");
    restore_cwd();
    system("rm -rf /tmp/cboot_test_interactive3");
    ENDCASE;

    CASE("g_force + 已存在目录 → 强制重建");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_interactive4");
    chdir("/tmp/cboot_test_interactive4");
    make_tmp_dir("forcedir");
    g_force = 1;
    g_running = 0;
    suppress_stdout(); suppress_stderr();
    int rc4 = main_run_interactive("forcedir", "cboot");
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc4, 0, "强制重建返回 0");
    restore_cwd();
    system("rm -rf /tmp/cboot_test_interactive4");
    ENDCASE;
}

/* ================================================================== */
/* 7. cboot_main (renamed main)                                        */
/* ================================================================== */
TEST_SUITE(test_main_entry) {
    char *argv_help[] = {"cboot", "--help"};
    char *argv_ver[] = {"cboot", "--version"};
    char *argv_unknown[] = {"cboot", "-x"};
    char *argv_noargs[] = {"cboot"};
    char *argv_script[] = {"cboot", "nonexist.cboot"};
    char *argv_analyze[] = {"cboot", "analyze"};

    CASE("--help 返回 0");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(cboot_main(2, argv_help), 0, "--help 返回 0");
    restore_stdout();
    ENDCASE;

    CASE("--version 返回 0");
    reset_state();
    suppress_stdout();
    ASSERT_EQ_INT(cboot_main(2, argv_ver), 0, "--version 返回 0");
    restore_stdout();
    ENDCASE;

    CASE("未知选项 -x 返回 1");
    reset_state();
    suppress_stdout(); suppress_stderr();
    ASSERT_EQ_INT(cboot_main(2, argv_unknown), 1, "-x 返回 1");
    restore_stderr(); restore_stdout();
    ENDCASE;

    CASE("无参数无 .cboot 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_main_entry");
    chdir("/tmp/cboot_test_main_entry");
    suppress_stdout(); suppress_stderr();
    int rc = cboot_main(1, argv_noargs);
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc, 1, "无参数无 .cboot 返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_main_entry");
    restore_cwd();
    ENDCASE;

    CASE("不存在的脚本返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_main_entry2");
    chdir("/tmp/cboot_test_main_entry2");
    suppress_stdout(); suppress_stderr();
    int rc2 = cboot_main(2, argv_script);
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc2, 1, "脚本不存在返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_main_entry2");
    restore_cwd();
    ENDCASE;

    CASE("analyze 模式无 .cboot 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_main_entry3");
    chdir("/tmp/cboot_test_main_entry3");
    suppress_stdout(); suppress_stderr();
    int rc3 = cboot_main(2, argv_analyze);
    restore_stderr(); restore_stdout();
    ASSERT_EQ_INT(rc3, 1, "analyze 无 .cboot 返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_main_entry3");
    restore_cwd();
    ENDCASE;
}

/* ================================================================== */
/* 8. main_detect_fine_tune_mode                                       */
/* ================================================================== */
TEST_SUITE(test_detect_fine_tune) {
    CASE("无文件返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_finetune");
    chdir("/tmp/cboot_test_finetune");
    ASSERT_EQ_INT(main_detect_fine_tune_mode(), 0, "无文件返回 0");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_finetune");
    restore_cwd();
    ENDCASE;

    CASE("只有 CMakeLists.txt 返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_finetune2");
    chdir("/tmp/cboot_test_finetune2");
    write_temp_file("CMakeLists.txt", "");
    ASSERT_EQ_INT(main_detect_fine_tune_mode(), 0, "缺 main.c 返回 0");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_finetune2");
    restore_cwd();
    ENDCASE;

    CASE("CMakeLists.txt + main.c 返回 1");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_finetune3");
    chdir("/tmp/cboot_test_finetune3");
    write_temp_file("CMakeLists.txt", "");
    write_temp_file("main.c", "");
    ASSERT_EQ_INT(main_detect_fine_tune_mode(), 1, "两文件都存在返回 1");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_finetune3");
    restore_cwd();
    ENDCASE;

    CASE("只有 main.c 返回 0");
    reset_state();
    save_cwd();
    make_tmp_dir("/tmp/cboot_test_finetune4");
    chdir("/tmp/cboot_test_finetune4");
    write_temp_file("main.c", "");
    ASSERT_EQ_INT(main_detect_fine_tune_mode(), 0, "缺 CMakeLists.txt 返回 0");
    chdir("/tmp"); system("rm -rf /tmp/cboot_test_finetune4");
    restore_cwd();
    ENDCASE;
}

/* ================================================================== */
/* 9. main_print_usage                                                 */
/* ================================================================== */
TEST_SUITE(test_print_usage) {
    CASE("main_print_usage 不崩溃");
    suppress_stdout();
    main_print_usage("cboot");
    restore_stdout();
    ENDCASE;
}

/* ================================================================== */
/* 10. main_try_complete                                               */
/* ================================================================== */
TEST_SUITE(test_try_complete) {
    const char *matches[128];
    int n;

    CASE("前缀 mo → mod, mode");
    n = main_try_complete("mo", g_cmd_names, 128, matches);
    ASSERT_EQ_INT(n, 2, "mo 匹配 2 个");
    ASSERT_EQ_STR(matches[0], "mod", "第一个 mod");
    ASSERT_EQ_STR(matches[1], "mode", "第二个 mode");
    ENDCASE;

    CASE("前缀 mod → mod, mode");
    n = main_try_complete("mod", g_cmd_names, 128, matches);
    ASSERT_EQ_INT(n, 2, "mod 匹配 2 个 (mod + mode)");
    ASSERT_EQ_STR(matches[0], "mod", "mod");
    ENDCASE;

    CASE("前缀 m → mod, mem, mode, mv");
    n = main_try_complete("m", g_cmd_names, 128, matches);
    ASSERT_EQ_INT(n, 4, "m 匹配 4 个");
    ENDCASE;

    CASE("空前缀 → 全部");
    n = main_try_complete("", g_cmd_names, 128, matches);
    ASSERT_TRUE(n > 20, "空前缀匹配大量命令");
    ENDCASE;

    CASE("无匹配前缀 → 0");
    n = main_try_complete("xyz", g_cmd_names, 128, matches);
    ASSERT_EQ_INT(n, 0, "xyz 无匹配");
    ENDCASE;

    CASE("max_matches 限制");
    n = main_try_complete("", g_cmd_names, 3, matches);
    ASSERT_EQ_INT(n, 3, "max_matches=3 限制为 3");
    ENDCASE;
}

/* ================================================================== */
/* 11. main_common_prefix_len                                          */
/* ================================================================== */
TEST_SUITE(test_common_prefix_len) {
    const char *strs1[] = {"mod", "mode"};
    const char *strs2[] = {"mod", "mem"};
    const char *strs3[] = {"hello"};
    const char *strs4[] = {"abc", "abd", "abe"};

    CASE("mod/mode 公共前缀 3");
    ASSERT_EQ_INT(main_common_prefix_len(strs1, 2), 3, "mod/mode → 3");
    ENDCASE;

    CASE("mod/mem 公共前缀 1");
    ASSERT_EQ_INT(main_common_prefix_len(strs2, 2), 1, "mod/mem → 1");
    ENDCASE;

    CASE("单个字符串 → 全长");
    ASSERT_EQ_INT(main_common_prefix_len(strs3, 1), 5, "hello → 5");
    ENDCASE;

    CASE("count=0 → 0");
    ASSERT_EQ_INT(main_common_prefix_len(strs1, 0), 0, "count=0 → 0");
    ENDCASE;

    CASE("三个字符串 abc/abd/abe → 2");
    ASSERT_EQ_INT(main_common_prefix_len(strs4, 3), 2, "abc/abd/abe → 2");
    ENDCASE;
}

/* ================================================================== */
/* 12. tab_collect_child_domains                                       */
/* ================================================================== */
TEST_SUITE(test_tab_collect_child) {
    const char *matches[128];
    int n;

    CASE("无子域 → 0");
    reset_proj();
    n = tab_collect_child_domains("x", matches, 128);
    ASSERT_EQ_INT(n, 0, "无子域 0 匹配");
    ENDCASE;

    CASE("有匹配前缀的子域");
    reset_proj();
    ModuleDomain *m1 = domain_module_domain_new("mod1");
    ModuleDomain *m2 = domain_module_domain_new("mod2");
    ModuleDomain *m3 = domain_module_domain_new("other");
    domain_domain_add_child(g_proj->current, (Domain *)m1);
    domain_domain_add_child(g_proj->current, (Domain *)m2);
    domain_domain_add_child(g_proj->current, (Domain *)m3);
    n = tab_collect_child_domains("mod", matches, 128);
    ASSERT_EQ_INT(n, 2, "mod 前缀匹配 2 个");
    ASSERT_EQ_STR(matches[0], "mod1", "第一个");
    ASSERT_EQ_STR(matches[1], "mod2", "第二个");
    ENDCASE;

    CASE("无匹配前缀");
    reset_proj();
    ModuleDomain *m4 = domain_module_domain_new("foo");
    domain_domain_add_child(g_proj->current, (Domain *)m4);
    n = tab_collect_child_domains("bar", matches, 128);
    ASSERT_EQ_INT(n, 0, "bar 无匹配");
    ENDCASE;

    CASE("max_matches 限制");
    reset_proj();
    for (int i = 0; i < 5; i++) {
        char name[32]; snprintf(name, sizeof(name), "mod%d", i);
        ModuleDomain *m = domain_module_domain_new(name);
        domain_domain_add_child(g_proj->current, (Domain *)m);
    }
    n = tab_collect_child_domains("mod", matches, 2);
    ASSERT_EQ_INT(n, 2, "max=2 限制为 2");
    ENDCASE;
}

/* ================================================================== */
/* 13. tab_base_len                                                    */
/* ================================================================== */
TEST_SUITE(test_tab_base_len) {
    CASE("无空格 → 0");
    ASSERT_EQ_INT(tab_base_len("mod"), 0, "无空格 → 0");
    ENDCASE;

    CASE("单空格 → 空格后位置");
    ASSERT_EQ_INT(tab_base_len("cd mo"), 3, "cd mo → 3");
    ENDCASE;

    CASE("多空格 → 最后空格后");
    ASSERT_EQ_INT(tab_base_len("find mod abc"), 9, "find mod abc → 9");
    ENDCASE;

    CASE("空串 → 0");
    ASSERT_EQ_INT(tab_base_len(""), 0, "空串 → 0");
    ENDCASE;
}

/* ================================================================== */
/* 14. tab_apply_single / tab_apply_multi                              */
/* ================================================================== */
TEST_SUITE(test_tab_apply) {
    CASE("tab_apply_single: mod 补全");
    char line[MAX_LINE_LEN] = "mod";
    int pos = 3;
    const char *prompt = "> ";
    suppress_stdout();
    int rc = tab_apply_single(line, &pos, prompt, "mod", "mod");
    restore_stdout();
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_STR(line, "mod ", "补全为 'mod '");
    ASSERT_EQ_INT(pos, 4, "pos=4");
    ENDCASE;

    CASE("tab_apply_single: cd 子域补全");
    char line2[MAX_LINE_LEN] = "cd mo";
    int pos2 = 5;
    suppress_stdout();
    int rc2 = tab_apply_single(line2, &pos2, "> ", "cd mo", "mod1");
    restore_stdout();
    ASSERT_EQ_INT(rc2, 1, "返回 1");
    ASSERT_EQ_STR(line2, "cd mod1 ", "补全为 'cd mod1 '");
    ASSERT_EQ_INT(pos2, 8, "pos=8");
    ENDCASE;

    CASE("tab_apply_multi: 多匹配公共前缀");
    char line3[MAX_LINE_LEN] = "m";
    int pos3 = 1;
    const char *mset[] = {"mod", "mem", "mode", "mv"};
    suppress_stdout();
    int rc3 = tab_apply_multi(line3, &pos3, "> ", "m", mset, 4);
    restore_stdout();
    ASSERT_EQ_INT(rc3, 1, "返回 1");
    /* 公共前缀 "m" 已在 line 中, new_cplen = 1 - (1-0) = 0, 不追加 */
    ASSERT_EQ_INT(pos3, 1, "pos 不变");
    ENDCASE;
}

/* ================================================================== */
/* 15. tab_collect_for_cmd                                             */
/* ================================================================== */
TEST_SUITE(test_tab_collect_for_cmd) {
    const char *matches[128];
    int n;

    CASE("cd → 收集子域");
    reset_proj();
    ModuleDomain *m = domain_module_domain_new("modA");
    domain_domain_add_child(g_proj->current, (Domain *)m);
    n = tab_collect_for_cmd("cd", "mo", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "cd mo → 1 匹配");
    ASSERT_EQ_STR(matches[0], "modA", "modA");
    ENDCASE;

    CASE("rm → 收集子域");
    reset_proj();
    ModuleDomain *m2 = domain_module_domain_new("foo");
    domain_domain_add_child(g_proj->current, (Domain *)m2);
    n = tab_collect_for_cmd("rm", "f", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "rm f → 1");
    ENDCASE;

    CASE("find tc=2 → g_domain_types");
    n = tab_collect_for_cmd("find", "mo", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "find mo → mod");
    ASSERT_EQ_STR(matches[0], "mod", "mod");
    ENDCASE;

    CASE("find tc=3 → 默认 (子域)");
    reset_proj();
    ModuleDomain *m3 = domain_module_domain_new("modX");
    domain_domain_add_child(g_proj->current, (Domain *)m3);
    n = tab_collect_for_cmd("find", "mod", 3, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "find tc=3 → 子域");
    ENDCASE;

    CASE("mode → g_mode_values");
    n = tab_collect_for_cmd("mode", "s", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 3, "mode s → src, static, struct");
    ENDCASE;

    CASE("cmode → g_cmode_values");
    n = tab_collect_for_cmd("cmode", "e", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "cmode e → exe");
    ASSERT_EQ_STR(matches[0], "exe", "exe");
    ENDCASE;

    CASE("未知 cmd → 默认子域");
    reset_proj();
    ModuleDomain *m4 = domain_module_domain_new("child");
    domain_domain_add_child(g_proj->current, (Domain *)m4);
    n = tab_collect_for_cmd("unknown", "ch", 2, 0, matches, 128);
    ASSERT_EQ_INT(n, 1, "未知 → 子域");
    ENDCASE;
}

/* ================================================================== */
/* 16. main_do_tab_complete                                            */
/* ================================================================== */
TEST_SUITE(test_do_tab_complete) {
    CASE("空行 Tab → 多匹配 (列出全部命令)");
    char line[MAX_LINE_LEN] = "";
    int pos = 0;
    suppress_stdout();
    int rc = main_do_tab_complete(line, &pos, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc, 1, "应刷新显示");
    ENDCASE;

    CASE("'stru' Tab → 单匹配补全 struct");
    char line2[MAX_LINE_LEN] = "stru";
    int pos2 = 4;
    suppress_stdout();
    int rc2 = main_do_tab_complete(line2, &pos2, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc2, 1, "返回 1");
    ASSERT_EQ_STR(line2, "struct ", "补全为 'struct '");
    ASSERT_EQ_INT(pos2, 7, "pos=7");
    ENDCASE;

    CASE("'xyz' Tab → 无匹配");
    char line3[MAX_LINE_LEN] = "xyz";
    int pos3 = 3;
    suppress_stdout();
    int rc3 = main_do_tab_complete(line3, &pos3, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc3, 0, "无匹配返回 0");
    ENDCASE;

    CASE("'mo' Tab → 多匹配公共前缀");
    char line4[MAX_LINE_LEN] = "mo";
    int pos4 = 2;
    suppress_stdout();
    int rc4 = main_do_tab_complete(line4, &pos4, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc4, 1, "多匹配返回 1");
    /* 公共前缀 "mod" → 追加 "d" */
    ASSERT_EQ_INT(pos4, 3, "pos=3 (追加 d)");
    ENDCASE;
}

/* ================================================================== */
/* 17. main_repl_history_up / down                                     */
/* ================================================================== */
TEST_SUITE(test_repl_history) {
    CASE("history_up: 空历史无操作");
    char line[MAX_LINE_LEN] = "abc";
    int pos = 3;
    char *history[5] = {0};
    int hist_count = 0, hist_pos = 0, hist_browsing = 0;
    char saved[MAX_LINE_LEN] = "";
    suppress_stdout();
    main_repl_history_up(line, &pos, "> ", history, hist_count, &hist_pos, &hist_browsing, saved);
    restore_stdout();
    ASSERT_EQ_INT(hist_browsing, 0, "无历史不进入 browsing");
    ENDCASE;

    CASE("history_up: 进入 browsing, 加载最后一条");
    char line2[MAX_LINE_LEN] = "cur";
    int pos2 = 3;
    char *hist2[5] = {(char*)"cmd1", (char*)"cmd2"};
    int hc2 = 2, hp2 = 0, hb2 = 0;
    char saved2[MAX_LINE_LEN] = "";
    suppress_stdout();
    main_repl_history_up(line2, &pos2, "> ", hist2, hc2, &hp2, &hb2, saved2);
    restore_stdout();
    ASSERT_EQ_INT(hb2, 1, "进入 browsing");
    ASSERT_EQ_INT(hp2, 1, "hist_pos=1 (最后一条)");
    ASSERT_EQ_STR(line2, "cmd2", "加载 cmd2");
    ASSERT_EQ_STR(saved2, "cur", "saved_line=cur");
    ENDCASE;

    CASE("history_up: 连续 up 浏览更早");
    char line3[MAX_LINE_LEN] = "x";
    int pos3 = 1;
    char *hist3[5] = {(char*)"old", (char*)"new"};
    int hc3 = 2, hp3 = 2, hb3 = 1;
    char saved3[MAX_LINE_LEN] = "x";
    suppress_stdout();
    main_repl_history_up(line3, &pos3, "> ", hist3, hc3, &hp3, &hb3, saved3);
    restore_stdout();
    ASSERT_EQ_INT(hp3, 1, "第一次 up → pos=1");
    ASSERT_EQ_STR(line3, "new", "加载 new");
    suppress_stdout();
    main_repl_history_up(line3, &pos3, "> ", hist3, hc3, &hp3, &hb3, saved3);
    restore_stdout();
    ASSERT_EQ_INT(hp3, 0, "第二次 up → pos=0");
    ASSERT_EQ_STR(line3, "old", "加载 old");
    ENDCASE;

    CASE("history_down: 非 browsing 无操作");
    char line4[MAX_LINE_LEN] = "abc";
    int pos4 = 3;
    int hb4 = 0;
    suppress_stdout();
    main_repl_history_down(line4, &pos4, "> ", NULL, 0, NULL, &hb4, NULL);
    restore_stdout();
    ASSERT_EQ_INT(hb4, 0, "非 browsing 不变");
    ENDCASE;

    CASE("history_down: 到末尾恢复 saved_line");
    char line5[MAX_LINE_LEN] = "old_cmd";
    int pos5 = 7;
    char *hist5[5] = {(char*)"old_cmd"};
    int hc5 = 1, hp5 = 0, hb5 = 1;
    char saved5[MAX_LINE_LEN] = "my_input";
    suppress_stdout();
    main_repl_history_down(line5, &pos5, "> ", hist5, hc5, &hp5, &hb5, saved5);
    restore_stdout();
    ASSERT_EQ_INT(hb5, 0, "退出 browsing");
    ASSERT_EQ_STR(line5, "my_input", "恢复 saved_line");
    ENDCASE;

    CASE("history_down: 中间位置向下浏览");
    char line6[MAX_LINE_LEN] = "";
    int pos6 = 0;
    char *hist6[5] = {(char*)"a", (char*)"b", (char*)"c"};
    int hc6 = 3, hp6 = 0, hb6 = 1;
    char saved6[MAX_LINE_LEN] = "";
    suppress_stdout();
    main_repl_history_down(line6, &pos6, "> ", hist6, hc6, &hp6, &hb6, saved6);
    restore_stdout();
    ASSERT_EQ_INT(hp6, 1, "pos=1");
    ASSERT_EQ_STR(line6, "b", "加载 b");
    ENDCASE;
}

/* ================================================================== */
/* 18. main_repl_handle_esc / handle_esc2                              */
/* ================================================================== */
TEST_SUITE(test_repl_handle_esc) {
    char line[MAX_LINE_LEN] = "";
    int pos = 0;
    char *history[5] = {0};
    int hist_count = 0, hist_pos = 0, hist_browsing = 0;
    char saved[MAX_LINE_LEN] = "";
    int esc_state, rc;

    CASE("ESC → state 1");
    reset_state();
    esc_state = 0;
    rc = main_repl_handle_esc(&esc_state, '\033', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 1, "state=1");
    ENDCASE;

    CASE("state 1 + '[' → state 2");
    esc_state = 1;
    rc = main_repl_handle_esc(&esc_state, '[', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 2, "state=2");
    ENDCASE;

    CASE("state 1 + 非'[' → state 0");
    esc_state = 1;
    rc = main_repl_handle_esc(&esc_state, 'x', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 0, "state=0");
    ENDCASE;

    CASE("state 2 + 'A' → history_up, state 0");
    {
        char *hist_up[5] = {(char*)"prev_cmd"};
        int hc_up = 1, hp_up = 0, hb_up = 0;
        char line_up[MAX_LINE_LEN] = "cur";
        int pos_up = 3;
        char saved_up[MAX_LINE_LEN] = "";
        esc_state = 2;
        suppress_stdout();
        rc = main_repl_handle_esc(&esc_state, 'A', line_up, &pos_up, "> ",
                                  hist_up, hc_up, &hp_up, &hb_up, saved_up);
        restore_stdout();
        ASSERT_EQ_INT(rc, 1, "返回 1");
        ASSERT_EQ_INT(esc_state, 0, "state=0");
        ASSERT_EQ_INT(hb_up, 1, "进入 browsing");
        ASSERT_EQ_STR(line_up, "prev_cmd", "加载历史");
    }
    ENDCASE;

    CASE("state 2 + 'B' → history_down, state 0");
    {
        char *hist_dn[5] = {(char*)"cmd"};
        int hc_dn = 1, hp_dn = 0;
        int hb_dn = 1;
        char line_dn[MAX_LINE_LEN] = "cmd";
        int pos_dn = 3;
        char saved_dn[MAX_LINE_LEN] = "orig";
        esc_state = 2;
        suppress_stdout();
        rc = main_repl_handle_esc(&esc_state, 'B', line_dn, &pos_dn, "> ",
                                  hist_dn, hc_dn, &hp_dn, &hb_dn, saved_dn);
        restore_stdout();
        ASSERT_EQ_INT(rc, 1, "返回 1");
        ASSERT_EQ_INT(esc_state, 0, "state=0");
        ASSERT_EQ_INT(hb_dn, 0, "退出 browsing (恢复 saved)");
    }
    ENDCASE;

    CASE("state 2 + '?' → state 3");
    esc_state = 2;
    rc = main_repl_handle_esc(&esc_state, '?', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 3, "state=3");
    ENDCASE;

    CASE("state 2 + 普通字符 → state 0");
    esc_state = 2;
    rc = main_repl_handle_esc(&esc_state, 'x', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 0, "state=0");
    ENDCASE;

    CASE("state 3 + 非数字 → state 0");
    esc_state = 3;
    rc = main_repl_handle_esc(&esc_state, 'x', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 1, "返回 1");
    ASSERT_EQ_INT(esc_state, 0, "state=0");
    ENDCASE;

    CASE("非 ESC 字符 + state 0 → 返回 0");
    esc_state = 0;
    rc = main_repl_handle_esc(&esc_state, 'a', line, &pos, "> ",
                              history, hist_count, &hist_pos, &hist_browsing, saved);
    ASSERT_EQ_INT(rc, 0, "非 ESC 返回 0");
    ASSERT_EQ_INT(esc_state, 0, "state 不变");
    ENDCASE;
}

/* ================================================================== */
/* 19. main_repl_handle_ctrl                                           */
/* ================================================================== */
TEST_SUITE(test_repl_handle_ctrl) {
    CASE("Tab + pos>0 → 调用 tab_complete 返回 1");
    reset_proj();
    char line[MAX_LINE_LEN] = "mod";
    int pos = 3;
    suppress_stdout();
    int rc = main_repl_handle_ctrl('\t', line, &pos, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc, 1, "Tab 返回 1");
    ENDCASE;

    CASE("Tab + pos=0 → 返回 1 (不补全)");
    char line2[MAX_LINE_LEN] = "";
    int pos2 = 0;
    int rc2 = main_repl_handle_ctrl('\t', line2, &pos2, "> ");
    ASSERT_EQ_INT(rc2, 1, "Tab pos=0 返回 1");
    ENDCASE;

    CASE("\\n → 返回 2");
    char line3[MAX_LINE_LEN] = "hello";
    int pos3 = 5;
    int rc3 = main_repl_handle_ctrl('\n', line3, &pos3, "> ");
    ASSERT_EQ_INT(rc3, 2, "\\n 返回 2");
    ASSERT_EQ_INT(line3[pos3], '\0', "line[pos] 置 0");
    ENDCASE;

    CASE("\\r → 返回 2");
    char line4[MAX_LINE_LEN] = "hi";
    int pos4 = 2;
    int rc4 = main_repl_handle_ctrl('\r', line4, &pos4, "> ");
    ASSERT_EQ_INT(rc4, 2, "\\r 返回 2");
    ENDCASE;

    CASE("Ctrl+C (3) → 清空行返回 1");
    char line5[MAX_LINE_LEN] = "text";
    int pos5 = 4;
    suppress_stdout();
    int rc5 = main_repl_handle_ctrl(3, line5, &pos5, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc5, 1, "Ctrl+C 返回 1");
    ASSERT_EQ_INT(pos5, 0, "pos=0");
    ASSERT_EQ_STR(line5, "", "行清空");
    ENDCASE;

    CASE("Ctrl+D (4) + pos=0 → g_running=0 返回 1");
    reset_state();
    char line6[MAX_LINE_LEN] = "";
    int pos6 = 0;
    suppress_stdout();
    int rc6 = main_repl_handle_ctrl(4, line6, &pos6, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc6, 1, "Ctrl+D 返回 1");
    ASSERT_EQ_INT(g_running, 0, "g_running=0");
    ENDCASE;

    CASE("Ctrl+D (4) + pos>0 → 不退出 返回 1");
    reset_state();
    char line7[MAX_LINE_LEN] = "text";
    int pos7 = 4;
    int rc7 = main_repl_handle_ctrl(4, line7, &pos7, "> ");
    ASSERT_EQ_INT(rc7, 1, "Ctrl+D 返回 1");
    ASSERT_EQ_INT(g_running, 1, "g_running 不变");
    ENDCASE;

    CASE("Ctrl+L (12) → 返回 1");
    char line8[MAX_LINE_LEN] = "text";
    int pos8 = 4;
    suppress_stdout();
    int rc8 = main_repl_handle_ctrl(12, line8, &pos8, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc8, 1, "Ctrl+L 返回 1");
    ENDCASE;

    CASE("Backspace (127) + pos>0 → 删除字符");
    char line9[MAX_LINE_LEN] = "abc";
    int pos9 = 3;
    suppress_stdout();
    int rc9 = main_repl_handle_ctrl(127, line9, &pos9, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc9, 1, "Backspace 返回 1");
    ASSERT_EQ_INT(pos9, 2, "pos=2");
    ENDCASE;

    CASE("Backspace + pos=0 → 返回 1 不删除");
    char line10[MAX_LINE_LEN] = "";
    int pos10 = 0;
    int rc10 = main_repl_handle_ctrl(127, line10, &pos10, "> ");
    ASSERT_EQ_INT(rc10, 1, "Backspace pos=0 返回 1");
    ASSERT_EQ_INT(pos10, 0, "pos 不变");
    ENDCASE;

    CASE("\\b (8) 同 backspace");
    char line11[MAX_LINE_LEN] = "xy";
    int pos11 = 2;
    suppress_stdout();
    int rc11 = main_repl_handle_ctrl('\b', line11, &pos11, "> ");
    restore_stdout();
    ASSERT_EQ_INT(rc11, 1, "\\b 返回 1");
    ASSERT_EQ_INT(pos11, 1, "pos=1");
    ENDCASE;

    CASE("普通字符 → 返回 0");
    char line12[MAX_LINE_LEN] = "";
    int pos12 = 0;
    int rc12 = main_repl_handle_ctrl('a', line12, &pos12, "> ");
    ASSERT_EQ_INT(rc12, 0, "普通字符返回 0");
    ENDCASE;
}

/* ================================================================== */
/* 20. main_repl_read_line                                             */
/* ================================================================== */
TEST_SUITE(test_repl_read_line) {
    CASE("读取 hello 行");
    reset_proj();
    g_running = 1;
    char line[MAX_LINE_LEN] = {0};
    char saved[MAX_LINE_LEN] = {0};
    char *history[5] = {0};
    int hist_count = 0, hist_pos = 0, hist_browsing = 0;
    feed_stdin("hello\n");
    suppress_stdout();
    int rc = main_repl_read_line(line, "> ", history, hist_count,
                                 &hist_pos, &hist_browsing, saved);
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(rc, 1, "读到换行返回 1");
    ASSERT_EQ_STR(line, "hello", "line=hello");
    ENDCASE;

    CASE("空输入 (EOF) → 返回 0, g_running=0");
    reset_proj();
    g_running = 1;
    char line2[MAX_LINE_LEN] = {0};
    char saved2[MAX_LINE_LEN] = {0};
    char *hist2[5] = {0};
    int hc2 = 0, hp2 = 0, hb2 = 0;
    feed_stdin("");
    suppress_stdout();
    int rc2 = main_repl_read_line(line2, "> ", hist2, hc2, &hp2, &hb2, saved2);
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(rc2, 0, "EOF 返回 0");
    ASSERT_EQ_INT(g_running, 0, "g_running=0");
    ENDCASE;

    CASE("Ctrl+D 在空行 → 返回 0");
    reset_proj();
    g_running = 1;
    char line3[MAX_LINE_LEN] = {0};
    char saved3[MAX_LINE_LEN] = {0};
    char *hist3[5] = {0};
    int hc3 = 0, hp3 = 0, hb3 = 0;
    feed_stdin("\x04");  /* Ctrl+D */
    suppress_stdout();
    int rc3 = main_repl_read_line(line3, "> ", hist3, hc3, &hp3, &hb3, saved3);
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(rc3, 0, "Ctrl+D 空行返回 0");
    ASSERT_EQ_INT(g_running, 0, "g_running=0");
    ENDCASE;
}

/* ================================================================== */
/* 21. main_repl_loop                                                  */
/* ================================================================== */
TEST_SUITE(test_repl_loop) {
    CASE("exit 命令退出 repl");
    reset_proj();
    g_running = 1;
    mock_reset();
    feed_stdin("exit\n");
    suppress_stdout();
    main_repl_loop();
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(g_running, 0, "g_running=0");
    ASSERT_EQ_INT(g_mock_exit_called, 1, "exit 被调用");
    ENDCASE;

    CASE("空行不触发命令");
    reset_proj();
    g_running = 1;
    mock_reset();
    feed_stdin("\nexit\n");
    suppress_stdout();
    main_repl_loop();
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(g_running, 0, "最终退出");
    ASSERT_EQ_INT(g_mock_exit_called, 1, "exit 被调用 1 次");
    ENDCASE;
}

/* ================================================================== */
/* 22. main_join_rest                                                  */
/* ================================================================== */
TEST_SUITE(test_join_rest) {
    char buf[MAX_LINE_LEN];
    CASE("单 token → 空串");
    const char *t1[] = {"cmd"};
    main_join_rest(buf, sizeof(buf), (char **)t1, 1);
    ASSERT_EQ_STR(buf, "", "单 token → 空");
    ENDCASE;

    CASE("两 token → 第二个");
    const char *t2[] = {"cmd", "arg1"};
    main_join_rest(buf, sizeof(buf), (char **)t2, 2);
    ASSERT_EQ_STR(buf, "arg1", "两 token → arg1");
    ENDCASE;

    CASE("多 token → 空格连接");
    const char *t3[] = {"cmd", "arg1", "arg2", "arg3"};
    main_join_rest(buf, sizeof(buf), (char **)t3, 4);
    ASSERT_EQ_STR(buf, "arg1 arg2 arg3", "多 token 空格连接");
    ENDCASE;
}

/* ================================================================== */
/* 23. main_dispatch_build                                             */
/* ================================================================== */
TEST_SUITE(test_dispatch_build) {
    CASE("mod 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t[] = {"mod"};
    suppress_stdout();
    int rc = main_dispatch_build("mod", (char **)t, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc, -1, "缺参数返回 -1");
    ASSERT_EQ_INT(g_mock_call_count, 0, "不应调用 mock");
    ENDCASE;

    CASE("mod 有参数 → 调用 commands_cmd_mod");
    reset_proj(); mock_reset();
    const char *t2[] = {"mod", "mymod"};
    suppress_stdout();
    int rc2 = main_dispatch_build("mod", (char **)t2, 2);
    restore_stdout();
    ASSERT_EQ_INT(rc2, 0, "返回 0");
    ASSERT_EQ_INT(g_mock_call_count, 1, "调用 1 次");
    ASSERT_EQ_STR(g_mock_last_cmd, "mod", "mod");
    ASSERT_EQ_STR(g_mock_last_arg1, "mymod", "arg1=mymod");
    ENDCASE;

    CASE("struct 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t3[] = {"struct", "S"};
    suppress_stdout();
    main_dispatch_build("struct", (char **)t3, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "struct", "struct");
    ENDCASE;

    CASE("void 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t4[] = {"void", "f"};
    suppress_stdout();
    int rc4 = main_dispatch_build("void", (char **)t4, 2);
    restore_stdout();
    ASSERT_EQ_INT(rc4, -1, "void 缺第三个参数");
    ENDCASE;

    CASE("void 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t5[] = {"void", "f", "int"};
    suppress_stdout();
    int rc5 = main_dispatch_build("void", (char **)t5, 3);
    restore_stdout();
    ASSERT_EQ_INT(rc5, 0, "返回 0");
    ASSERT_EQ_STR(g_mock_last_cmd, "void", "void");
    ASSERT_EQ_STR(g_mock_last_arg1, "f", "name=f");
    ASSERT_EQ_STR(g_mock_last_arg2, "int", "type=int");
    ENDCASE;

    CASE("enum 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t6[] = {"enum", "A,B", "0"};
    suppress_stdout();
    main_dispatch_build("enum", (char **)t6, 3);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "enum", "enum");
    ENDCASE;

    CASE("未知命令 → -2");
    reset_proj(); mock_reset();
    const char *t7[] = {"unknown"};
    int rc7 = main_dispatch_build("unknown", (char **)t7, 1);
    ASSERT_EQ_INT(rc7, -2, "未知返回 -2");
    ENDCASE;
}

/* ================================================================== */
/* 24. main_read_code_from_stdin                                       */
/* ================================================================== */
TEST_SUITE(test_read_code) {
    char buf[MAX_LINE_LEN * 64];
    CASE("空输入 → 空串");
    feed_stdin("");
    main_read_code_from_stdin(buf, sizeof(buf));
    restore_stdin();
    ASSERT_EQ_STR(buf, "", "空输入 → 空串");
    ENDCASE;

    CASE("单行 + EOF → 该行");
    feed_stdin("int x = 0;\nEOF\n");
    main_read_code_from_stdin(buf, sizeof(buf));
    restore_stdin();
    ASSERT_EQ_STR(buf, "int x = 0;", "单行代码");
    ENDCASE;

    CASE("多行 + EOF → 换行连接");
    feed_stdin("line1\nline2\nline3\nEOF\n");
    main_read_code_from_stdin(buf, sizeof(buf));
    restore_stdin();
    ASSERT_EQ_STR(buf, "line1\nline2\nline3", "多行换行连接");
    ENDCASE;
}

/* ================================================================== */
/* 25. main_dispatch_modify                                            */
/* ================================================================== */
TEST_SUITE(test_dispatch_modify) {
    CASE("value 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t[] = {"value"};
    suppress_stdout();
    int rc = main_dispatch_modify("value", (char **)t, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc, -1, "缺参数");
    ENDCASE;

    CASE("value 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t2[] = {"value", "42"};
    suppress_stdout();
    main_dispatch_modify("value", (char **)t2, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "value", "value");
    ASSERT_EQ_STR(g_mock_last_arg1, "42", "42");
    ENDCASE;

    CASE("cmode 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t3[] = {"cmode", "exe"};
    suppress_stdout();
    main_dispatch_modify("cmode", (char **)t3, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "cmode", "cmode");
    ENDCASE;

    CASE("cmt 去引号 → 调用");
    reset_proj(); mock_reset();
    const char *t4[] = {"cmt", "\"hello world\""};
    suppress_stdout();
    main_dispatch_modify("cmt", (char **)t4, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "cmt", "cmt");
    ASSERT_EQ_STR(g_mock_last_arg1, "hello world", "去引号");
    ENDCASE;

    CASE("code 有参数 → 设置 code");
    reset_proj(); mock_reset();
    const char *t5[] = {"code", "int", "x", "=", "0;"};
    suppress_stdout();
    int rc5 = main_dispatch_modify("code", (char **)t5, 5);
    restore_stdout();
    ASSERT_EQ_INT(rc5, 0, "返回 0");
    ModuleDomain *mod = (ModuleDomain *)g_proj->current;
    ASSERT_NOT_NULL(mod->code, "code 已设置");
    ASSERT_EQ_STR(mod->code, "int x = 0;", "code 内容正确");
    ENDCASE;

    CASE("code 无参数 → 从 stdin 读取");
    reset_proj(); mock_reset();
    const char *t6[] = {"code"};
    feed_stdin("x = 1;\nEOF\n");
    suppress_stdout();
    int rc6 = main_dispatch_modify("code", (char **)t6, 1);
    restore_stdout();
    restore_stdin();
    ASSERT_EQ_INT(rc6, 0, "返回 0");
    ModuleDomain *mod2 = (ModuleDomain *)g_proj->current;
    ASSERT_EQ_STR(mod2->code, "x = 1;", "code 从 stdin 读取");
    ENDCASE;

    CASE("未知命令 → -2");
    reset_proj(); mock_reset();
    const char *t7[] = {"unknown"};
    int rc7 = main_dispatch_modify("unknown", (char **)t7, 1);
    ASSERT_EQ_INT(rc7, -2, "未知返回 -2");
    ENDCASE;
}

/* ================================================================== */
/* 26. main_parse_rm                                                   */
/* ================================================================== */
TEST_SUITE(test_parse_rm) {
    const char *name; int force; int rc;
    CASE("仅 rm → -1");
    const char *t[] = {"rm"};
    suppress_stdout();
    rc = main_parse_rm((char **)t, 1, &name, &force);
    restore_stdout();
    ASSERT_EQ_INT(rc, -1, "仅 rm 返回 -1");
    ENDCASE;

    CASE("rm name → name 设置, force=0");
    const char *t2[] = {"rm", "target"};
    name = NULL; force = -1;
    int rc2 = main_parse_rm((char **)t2, 2, &name, &force);
    ASSERT_EQ_INT(rc2, 0, "返回 0");
    ASSERT_EQ_STR(name, "target", "name=target");
    ASSERT_EQ_INT(force, 0, "force=0");
    ENDCASE;

    CASE("rm -f name → force=1");
    const char *t3[] = {"rm", "-f", "target"};
    name = NULL; force = 0;
    int rc3 = main_parse_rm((char **)t3, 3, &name, &force);
    ASSERT_EQ_INT(rc3, 0, "返回 0");
    ASSERT_EQ_STR(name, "target", "name=target");
    ASSERT_EQ_INT(force, 1, "force=1");
    ENDCASE;

    CASE("rm name -f → force=1 (顺序无关)");
    const char *t4[] = {"rm", "target", "-f"};
    name = NULL; force = 0;
    int rc4 = main_parse_rm((char **)t4, 3, &name, &force);
    ASSERT_EQ_INT(rc4, 0, "返回 0");
    ASSERT_EQ_STR(name, "target", "name=target");
    ASSERT_EQ_INT(force, 1, "force=1");
    ENDCASE;
}

/* ================================================================== */
/* 27. main_parse_find_flags                                           */
/* ================================================================== */
TEST_SUITE(test_parse_find_flags) {
    CASE("无 flag → 0");
    const char *t[] = {"find", "mod", "abc"};
    ASSERT_EQ_INT(main_parse_find_flags((char **)t, 3), 0, "无 flag → 0");
    ENDCASE;

    CASE("-a → 1");
    const char *t2[] = {"find", "mod", "abc", "-a"};
    ASSERT_EQ_INT(main_parse_find_flags((char **)t2, 4), 1, "-a → 1");
    ENDCASE;

    CASE("-a5 → 5");
    const char *t3[] = {"find", "mod", "abc", "-a5"};
    ASSERT_EQ_INT(main_parse_find_flags((char **)t3, 4), 5, "-a5 → 5");
    ENDCASE;

    CASE("多个 flag → 最后一个");
    const char *t4[] = {"find", "mod", "abc", "-a", "-a3"};
    ASSERT_EQ_INT(main_parse_find_flags((char **)t4, 5), 3, "最后 -a3 → 3");
    ENDCASE;
}

/* ================================================================== */
/* 28. main_dispatch_control                                           */
/* ================================================================== */
TEST_SUITE(test_dispatch_control) {
    CASE("cd → 调用 commands_cmd_cd");
    reset_proj(); mock_reset();
    const char *t[] = {"cd", "mod"};
    suppress_stdout();
    main_dispatch_control("cd", (char **)t, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "cd", "cd");
    ASSERT_EQ_STR(g_mock_last_arg1, "mod", "arg=mod");
    ENDCASE;

    CASE("cd 无参数 → NULL 参数");
    reset_proj(); mock_reset();
    const char *t2[] = {"cd"};
    suppress_stdout();
    main_dispatch_control("cd", (char **)t2, 1);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "cd", "cd");
    ASSERT_NULL(g_mock_last_arg1, "arg=NULL");
    ENDCASE;

    CASE("ls → 调用 commands_cmd_ls");
    reset_proj(); mock_reset();
    const char *t3[] = {"ls"};
    suppress_stdout();
    main_dispatch_control("ls", (char **)t3, 1);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "ls", "ls");
    ENDCASE;

    CASE("exit → 调用 commands_cmd_exit");
    reset_proj(); mock_reset(); g_running = 1;
    const char *t4[] = {"exit"};
    main_dispatch_control("exit", (char **)t4, 1);
    ASSERT_EQ_INT(g_mock_exit_called, 1, "exit 被调用");
    ENDCASE;

    CASE("mv 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t5[] = {"mv", "src"};
    suppress_stdout();
    int rc5 = main_dispatch_control("mv", (char **)t5, 2);
    restore_stdout();
    ASSERT_EQ_INT(rc5, -1, "mv 缺参数");
    ENDCASE;

    CASE("mv 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t6[] = {"mv", "src", "tgt"};
    suppress_stdout();
    main_dispatch_control("mv", (char **)t6, 3);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "mv", "mv");
    ASSERT_EQ_STR(g_mock_last_arg1, "src", "src");
    ASSERT_EQ_STR(g_mock_last_arg2, "tgt", "tgt");
    ENDCASE;

    CASE("rm 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t7[] = {"rm", "name", "-f"};
    suppress_stdout();
    main_dispatch_control("rm", (char **)t7, 3);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "rm", "rm");
    ASSERT_EQ_STR(g_mock_last_arg1, "name", "name");
    ASSERT_EQ_INT(g_mock_last_int, 1, "force=1");
    ENDCASE;

    CASE("find 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t8[] = {"find", "mod"};
    suppress_stdout();
    int rc8 = main_dispatch_control("find", (char **)t8, 2);
    restore_stdout();
    ASSERT_EQ_INT(rc8, -1, "find 缺参数");
    ENDCASE;

    CASE("find 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t9[] = {"find", "mod", "abc", "-a"};
    suppress_stdout();
    main_dispatch_control("find", (char **)t9, 4);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "find", "find");
    ASSERT_EQ_STR(g_mock_last_arg1, "mod", "type=mod");
    ASSERT_EQ_STR(g_mock_last_arg2, "abc", "pattern=abc");
    ASSERT_EQ_INT(g_mock_last_int, 1, "flags=1");
    ENDCASE;

    CASE("未知命令 → -2");
    reset_proj(); mock_reset();
    const char *t10[] = {"unknown"};
    int rc10 = main_dispatch_control("unknown", (char **)t10, 1);
    ASSERT_EQ_INT(rc10, -2, "未知返回 -2");
    ENDCASE;
}

/* ================================================================== */
/* 29. main_dispatch_action                                            */
/* ================================================================== */
TEST_SUITE(test_dispatch_action) {
    CASE("gen → 调用 commands_cmd_gen");
    reset_proj(); mock_reset(); g_skip_gen = 0;
    const char *t[] = {"gen"};
    suppress_stdout();
    main_dispatch_action("gen", (char **)t, 1);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "gen", "gen");
    ENDCASE;

    CASE("gen + g_skip_gen=1 → 不调用返回 0");
    reset_proj(); mock_reset(); g_skip_gen = 1;
    const char *t2[] = {"gen"};
    int rc2 = main_dispatch_action("gen", (char **)t2, 1);
    ASSERT_EQ_INT(rc2, 0, "skip_gen 返回 0");
    ASSERT_EQ_INT(g_mock_call_count, 0, "不应调用");
    ENDCASE;

    CASE("analyze → 调用");
    reset_proj(); mock_reset();
    const char *t3[] = {"analyze"};
    main_dispatch_action("analyze", (char **)t3, 1);
    ASSERT_EQ_STR(g_mock_last_cmd, "analyze", "analyze");
    ENDCASE;

    CASE("update → 调用");
    reset_proj(); mock_reset();
    const char *t4[] = {"update"};
    main_dispatch_action("update", (char **)t4, 1);
    ASSERT_EQ_STR(g_mock_last_cmd, "update", "update");
    ENDCASE;

    CASE("adjust → 调用");
    reset_proj(); mock_reset();
    const char *t5[] = {"adjust"};
    main_dispatch_action("adjust", (char **)t5, 1);
    ASSERT_EQ_STR(g_mock_last_cmd, "adjust", "adjust");
    ENDCASE;

    CASE("im 缺参数 → -1");
    reset_proj(); mock_reset();
    const char *t6[] = {"im"};
    suppress_stdout();
    int rc6 = main_dispatch_action("im", (char **)t6, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc6, -1, "im 缺参数");
    ENDCASE;

    CASE("im 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t7[] = {"im", "file.cboot"};
    suppress_stdout();
    main_dispatch_action("im", (char **)t7, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "im", "im");
    ASSERT_EQ_STR(g_mock_last_arg1, "file.cboot", "file.cboot");
    ENDCASE;

    CASE("in 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t8[] = {"in", "proj.cboot"};
    suppress_stdout();
    main_dispatch_action("in", (char **)t8, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "in", "in");
    ENDCASE;

    CASE("res 有参数 → 调用");
    reset_proj(); mock_reset();
    const char *t9[] = {"res", "data.txt"};
    suppress_stdout();
    main_dispatch_action("res", (char **)t9, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "res", "res");
    ENDCASE;

    CASE("未知命令 → -2");
    reset_proj(); mock_reset();
    const char *t10[] = {"unknown"};
    int rc10 = main_dispatch_action("unknown", (char **)t10, 1);
    ASSERT_EQ_INT(rc10, -2, "未知返回 -2");
    ENDCASE;
}

/* ================================================================== */
/* 30. main_dispatch_command                                           */
/* ================================================================== */
TEST_SUITE(test_dispatch_command) {
    CASE("help → 打印 usage 返回 0");
    reset_proj(); mock_reset();
    const char *t[] = {"help"};
    suppress_stdout();
    int rc = main_dispatch_command((char **)t, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc, 0, "help 返回 0");
    ENDCASE;

    CASE("? → 同 help");
    reset_proj(); mock_reset();
    const char *t2[] = {"?"};
    suppress_stdout();
    int rc2 = main_dispatch_command((char **)t2, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc2, 0, "? 返回 0");
    ENDCASE;

    CASE("mod x → 分发到 build");
    reset_proj(); mock_reset();
    const char *t3[] = {"mod", "x"};
    suppress_stdout();
    main_dispatch_command((char **)t3, 2);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "mod", "mod 被分发");
    ENDCASE;

    CASE("exit → 分发到 control");
    reset_proj(); mock_reset(); g_running = 1;
    const char *t4[] = {"exit"};
    main_dispatch_command((char **)t4, 1);
    ASSERT_EQ_INT(g_mock_exit_called, 1, "exit 分发成功");
    ENDCASE;

    CASE("gen → 分发到 action");
    reset_proj(); mock_reset(); g_skip_gen = 0;
    const char *t5[] = {"gen"};
    suppress_stdout();
    main_dispatch_command((char **)t5, 1);
    restore_stdout();
    ASSERT_EQ_STR(g_mock_last_cmd, "gen", "gen 分发成功");
    ENDCASE;

    CASE(".cboot 引用 → parser_try_cboot_ref 返回 1 时返回 0");
    reset_proj(); mock_reset();
    g_mock_try_ref_return = 1;
    const char *t6[] = {".cboot"};
    int rc6 = main_dispatch_command((char **)t6, 1);
    ASSERT_EQ_INT(rc6, 0, ".cboot 引用返回 0");
    ASSERT_EQ_INT(g_mock_try_ref_calls, 1, "try_ref 被调用");
    ENDCASE;

    CASE(".cboot 非引用 → try_ref 返回 0, 未知命令");
    reset_proj(); mock_reset();
    g_mock_try_ref_return = 0;
    const char *t7[] = {".cboot"};
    suppress_stdout();
    int rc7 = main_dispatch_command((char **)t7, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc7, -1, "非引用未知命令返回 -1");
    ENDCASE;

    CASE("未知命令 → -1");
    reset_proj(); mock_reset();
    g_mock_try_ref_return = 0;
    const char *t8[] = {"unknown_cmd"};
    suppress_stdout();
    int rc8 = main_dispatch_command((char **)t8, 1);
    restore_stdout();
    ASSERT_EQ_INT(rc8, -1, "未知命令返回 -1");
    ENDCASE;
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */
int main(void) {
    save_cwd();

    RUN_SUITE(test_parse_flag);
    RUN_SUITE(test_parse_info_flag);
    RUN_SUITE(test_parse_positional);
    RUN_SUITE(test_parse_args);
    RUN_SUITE(test_run_modes);
    RUN_SUITE(test_run_interactive);
    RUN_SUITE(test_main_entry);
    RUN_SUITE(test_detect_fine_tune);
    RUN_SUITE(test_print_usage);
    RUN_SUITE(test_try_complete);
    RUN_SUITE(test_common_prefix_len);
    RUN_SUITE(test_tab_collect_child);
    RUN_SUITE(test_tab_base_len);
    RUN_SUITE(test_tab_apply);
    RUN_SUITE(test_tab_collect_for_cmd);
    RUN_SUITE(test_do_tab_complete);
    RUN_SUITE(test_repl_history);
    RUN_SUITE(test_repl_handle_esc);
    RUN_SUITE(test_repl_handle_ctrl);
    RUN_SUITE(test_repl_read_line);
    RUN_SUITE(test_repl_loop);
    RUN_SUITE(test_join_rest);
    RUN_SUITE(test_dispatch_build);
    RUN_SUITE(test_read_code);
    RUN_SUITE(test_dispatch_modify);
    RUN_SUITE(test_parse_rm);
    RUN_SUITE(test_parse_find_flags);
    RUN_SUITE(test_dispatch_control);
    RUN_SUITE(test_dispatch_action);
    RUN_SUITE(test_dispatch_command);

    restore_cwd();
    test_summary();
    return 0;
}
