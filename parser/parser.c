/* parser.c - CBoot generated (compiler: normal) */
/* Module: parser */

/*
 * CBoot - C Project Bootstrapping Tool v1.1.0
 * .cboot script parser (新规范 v1.1.0)
 *
 * 支持 .cboot 文件引用语法:
 *   <dir>/.cboot  — 在当前作用域创建/进入 <dir> 模块，解析 <dir>/.cboot 文件
 *   例如: b/.cboot 表示 mod b; cd b; 解析 b/.cboot; cd ..
 *   多级路径: b/c/.cboot 表示 mod b; cd b; mod c; cd c; 解析 b/c/.cboot; cd ..; cd ..
 *   纯 .cboot (无目录) — 在当前作用域直接解析 .cboot 文件
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Script directory tracking (for .cboot file reference resolution)    */
/* ------------------------------------------------------------------ */

char g_script_dir[MAX_PATH_LEN] = ".";

/* Check if a token is a .cboot file reference (e.g. "b/.cboot" or ".cboot") */
static int parser_is_cboot_ref(const char *token) {
    if (!token) return 0;
    int len = (int)strlen(token);
    if (len < 6) return 0;  /* ".cboot" is 6 chars */
    return strcmp(token + len - 6, ".cboot") == 0;
}

/* Forward declaration */
static int parser_exec_cboot_ref(const char *ref);

/* Public interface: check if token is a .cboot reference and execute it.
 * Returns 1 if it was a reference (and was executed), 0 if not a reference. */
int parser_try_cboot_ref(const char *token) {
    if (!parser_is_cboot_ref(token)) return 0;
    parser_exec_cboot_ref(token);
    return 1;
}

/* Execute a .cboot file reference:
 *   - Parse the directory part to create/enter modules
 *   - Parse the referenced .cboot file (g_script_dir managed by parser_parse_cboot_script)
 *   - Navigate back */
static int parser_exec_cboot_ref(const char *ref) {
    if (!ref) return -1;

    /* 保存当前作用域，确保无论脚本如何改变作用域都能恢复 */
    Domain *saved_scope = g_proj->current;

    /* Extract directory part and file part */
    char buf[MAX_PATH_LEN];
    strncpy(buf, ref, MAX_PATH_LEN - 1);
    buf[MAX_PATH_LEN - 1] = '\0';

    /* Find the last '/' */
    char *last_slash = strrchr(buf, '/');
    if (!last_slash) {
        /* No directory — just parse the file in current scope */
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", g_script_dir, ref);
        int ret = parser_parse_cboot_script(full_path);
        /* 恢复作用域（脚本可能通过 cd 命令改变了它） */
        g_proj->current = saved_scope;
        return ret;
    }

    /* Split: dir part = everything before last '/', file = last_slash+1 */
    *last_slash = '\0';
    const char *dir_part = buf;   /* e.g. "b" or "b/c" */
    const char *file_part = last_slash + 1;  /* ".cboot" */

    /* Resolve the full file path relative to current script dir */
    char full_path[MAX_PATH_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s/%s", g_script_dir, dir_part, file_part);

    /* Navigate into modules: split dir_part by '/' */
    int depth_entered = 0;
    char dir_copy[MAX_PATH_LEN];
    strncpy(dir_copy, dir_part, MAX_PATH_LEN - 1);
    dir_copy[MAX_PATH_LEN - 1] = '\0';

    char *saveptr = NULL;
    char *seg = strtok_r(dir_copy, "/", &saveptr);
    while (seg) {
        /* Create module if it doesn't exist */
        Domain *existing = domain_domain_find_child(g_proj->current, seg);
        if (!existing) {
            if (commands_cmd_mod(seg) != 0) {
                /* Rollback: restore scope */
                g_proj->current = saved_scope;
                return -1;
            }
        }
        /* cd into the module */
        if (commands_cmd_cd(seg) != 0) {
            g_proj->current = saved_scope;
            return -1;
        }
        depth_entered++;
        seg = strtok_r(NULL, "/", &saveptr);
    }

    /* Parse the referenced .cboot file (parser_parse_cboot_script manages g_script_dir) */
    int ret = parser_parse_cboot_script(full_path);

    /* 恢复作用域：无论脚本如何 cd，都回到调用前的状态。
     * 之前用 depth_entered 次 cd(..) 的方式在脚本中途出错时
     * 会因 depth 不匹配而恢复到错误的位置。 */
    g_proj->current = saved_scope;

    return ret;
}

/* ------------------------------------------------------------------ */
/* Script line dispatch                                                */
/* ------------------------------------------------------------------ */

/* Sentinel value indicating a category handler did not match the command.
 * Valid command return values are -1, 0, 1, 2, so this is safely out of range. */
#define PARSER_NOT_HANDLED  (-1000)

/* Build commands: project, mod, struct, type, def, void, var, mem, enum */
/* 拼接 tokens 从 start 开始的所有 token，用空格分隔 */
static void parser_join_tokens_from(char **tokens, int count, int start, char *buf, size_t size) {
    buf[0] = '\0';
    for (int i = start; i < count; i++) {
        if (i > start) strncat(buf, " ", size - strlen(buf) - 1);
        strncat(buf, tokens[i], size - strlen(buf) - 1);
    }
}

/* 检查参数数量；不足则打印用法并返回 -1 */
static int parser_require_args(char **tokens, int count, int needed, const char *usage) {
    if (count < needed) {
        fprintf(stderr, "parser: 用法: %s\n", usage);
        return -1;
    }
    return 0;
}

static int parser_dispatch_build(char **tokens, int count) {
    const char *cmd = tokens[0];

    /* project <name> */
    if (utils_str_eq(cmd, "project")) {
        if (parser_require_args(tokens, count, 2, "project <名称>") < 0) return -1;
        free(g_proj->name);
        g_proj->name = utils_str_dup(tokens[1]);
        if (g_proj->root) {
            free(g_proj->root->name);
            g_proj->root->name = utils_str_dup(tokens[1]);
        }
        return 0;
    }

    /* 单参数建立域命令表 */
    struct { const char *name; int (*fn)(const char *); const char *usage; } single_cmds[] = {
        {"mod",    commands_cmd_mod,    "mod <名称>"},
        {"struct", commands_cmd_struct, "struct <名称>"},
        {"type",   commands_cmd_type,   "type <名称>"},
        {"def",    commands_cmd_def,    "def <名称>"},
    };
    for (size_t i = 0; i < sizeof(single_cmds)/sizeof(single_cmds[0]); i++) {
        if (utils_str_eq(cmd, single_cmds[i].name)) {
            if (parser_require_args(tokens, count, 2, single_cmds[i].usage) < 0) return -1;
            return single_cmds[i].fn(tokens[1]);
        }
    }

    /* 双参数建立域命令 (name + type) */
    struct { const char *name; int (*fn)(const char *, const char *); const char *usage; } type_cmds[] = {
        {"void", commands_cmd_void, "void <名称> <返回类型>"},
        {"var",  commands_cmd_var,  "var <名称> <类型>"},
        {"mem",  commands_cmd_mem,  "mem <名称> <类型>"},
    };
    for (size_t i = 0; i < sizeof(type_cmds)/sizeof(type_cmds[0]); i++) {
        if (utils_str_eq(cmd, type_cmds[i].name)) {
            if (parser_require_args(tokens, count, 3, type_cmds[i].usage) < 0) return -1;
            char type_buf[MAX_LINE_LEN];
            parser_join_tokens_from(tokens, count, 2, type_buf, sizeof(type_buf));
            return type_cmds[i].fn(tokens[1], type_buf);
        }
    }

    /* 枚举式def: enum <def1>,<def2>,... <start_num> */
    if (utils_str_eq(cmd, "enum")) {
        if (parser_require_args(tokens, count, 3, "enum <def1>,<def2>,... <start_num>") < 0) return -1;
        return commands_cmd_enum(tokens[1], tokens[2]);
    }

    return PARSER_NOT_HANDLED;
}

/* Modify commands: cmt, value, call, mode, cmode, code */
static int parser_dispatch_modify(char **tokens, int count) {
    const char *cmd = tokens[0];

    /* 单参数修改命令表 (使用 tokens[1]) */
    struct { const char *name; int (*fn)(const char *); const char *usage; } single[] = {
        {"value", commands_cmd_value, "value <值>"},
        {"cmode", commands_cmd_cmode, "cmode <exe|sl|dl|normal>"},
    };
    for (size_t i = 0; i < sizeof(single)/sizeof(single[0]); i++) {
        if (utils_str_eq(cmd, single[i].name)) {
            if (parser_require_args(tokens, count, 2, single[i].usage) < 0) return -1;
            return single[i].fn(tokens[1]);
        }
    }

    /* 拼接式修改命令表 (拼接 tokens[1..] ) */
    struct { const char *name; int (*fn)(const char *); const char *usage; int strip_quotes; } joined[] = {
        {"cmt",  commands_cmd_cmt,  "cmt \"文本\"", 1},
        {"call", commands_cmd_call, "call <调用约定>", 0},
        {"mode", commands_cmd_mode, "mode <模式>", 0},
    };
    for (size_t i = 0; i < sizeof(joined)/sizeof(joined[0]); i++) {
        if (utils_str_eq(cmd, joined[i].name)) {
            if (parser_require_args(tokens, count, 2, joined[i].usage) < 0) return -1;
            char buf[MAX_LINE_LEN];
            parser_join_tokens_from(tokens, count, 1, buf, sizeof(buf));
            if (joined[i].strip_quotes) utils_strip_quotes(buf);
            return joined[i].fn(buf);
        }
    }

    /* 测试用例: test <inputs> => <expected> (纯函数, 可选)
     * inputs 为逗号分隔的输入表达式(对应 mem 参数顺序)
     * expected 为预期返回值表达式
     * 例: test 3,5 => 8  表示 add(3,5) 预期返回 8 */
    if (utils_str_eq(cmd, "test")) {
        if (parser_require_args(tokens, count, 2, "test <输入> => <预期>") < 0) return -1;
        char buf[MAX_LINE_LEN];
        parser_join_tokens_from(tokens, count, 1, buf, sizeof(buf));
        return commands_cmd_test(buf);
    }

    /* 测试用例: tcode <<EOF ... EOF (自定义代码, 可选)
     * 用于指针修改/IO 等复杂场景, 用户写 C 代码片段
     * 提供 EXPECT(cond, fmt, ...) 宏做断言 */
    if (utils_str_eq(cmd, "tcode")) {
        if (count >= 2 && !utils_str_eq(tokens[1], "<<EOF")) {
            char code_buf[MAX_LINE_LEN];
            parser_join_tokens_from(tokens, count, 1, code_buf, sizeof(code_buf));
            return commands_cmd_tcode(code_buf);
        }
        /* tcode <<EOF 由外层循环处理 (需要读取多行) */
        return 3;  /* 特殊返回值: 由外层处理 tcode heredoc */
    }

    /* 修改字段: code (单行，兼容) */
    if (utils_str_eq(cmd, "code")) {
        if (count >= 2 && !utils_str_eq(tokens[1], "<<EOF")) {
            char code_buf[MAX_LINE_LEN];
            parser_join_tokens_from(tokens, count, 1, code_buf, sizeof(code_buf));
            domain_domain_set_code(g_proj->current, code_buf);
            return 0;
        }
        /* code <<EOF 由外层循环处理（需要读取多行） */
        return 1;  /* 特殊返回值: 由外层处理 heredoc */
    }

    return PARSER_NOT_HANDLED;
}

/* Control commands: cd, rm, find, ls, mv, exit */
/* 解析 rm 命令的 -f 标志和名称 */
static int parser_parse_rm(char **tokens, int count, const char **name, int *force) {
    *name = NULL; *force = 0;
    for (int i = 1; i < count; i++) {
        if (utils_str_eq(tokens[i], "-f")) *force = 1;
        else *name = tokens[i];
    }
    if (!*name) { fprintf(stderr, "parser: 用法: rm <名称> [-f]\n"); return -1; }
    return 0;
}

/* 解析 find 命令的 -a/-an 标志 */
static int parser_parse_find_flags(char **tokens, int count) {
    int flags = 0;
    for (int i = 3; i < count; i++) {
        if (utils_str_eq(tokens[i], "-a")) flags = 1;
        else if (utils_str_startswith(tokens[i], "-a")) flags = atoi(tokens[i] + 2);
    }
    return flags;
}

static int parser_dispatch_control(char **tokens, int count) {
    const char *cmd = tokens[0];

    if (utils_str_eq(cmd, "cd")) {
        return commands_cmd_cd(count >= 2 ? tokens[1] : NULL);
    }
    if (utils_str_eq(cmd, "ls")) {
        return commands_cmd_ls(count >= 2 ? tokens[1] : NULL);
    }
    if (utils_str_eq(cmd, "exit")) {
        g_running = 0;
        return 0;
    }
    if (utils_str_eq(cmd, "mv")) {
        if (count < 3) { fprintf(stderr, "parser: 用法: mv <src> <target>\n"); return -1; }
        return commands_cmd_mv(tokens[1], tokens[2]);
    }
    if (utils_str_eq(cmd, "rm")) {
        const char *name; int force;
        if (parser_parse_rm(tokens, count, &name, &force) != 0) return -1;
        return commands_cmd_rm(name, force);
    }
    if (utils_str_eq(cmd, "find")) {
        if (count < 3) { fprintf(stderr, "parser: 用法: find <类型> <字符> [-a|-an]\n"); return -1; }
        return commands_cmd_find(tokens[1], tokens[2], parser_parse_find_flags(tokens, count));
    }

    return PARSER_NOT_HANDLED;
}

/* Action commands: gen, analyze, update, im, in, res */
static int parser_dispatch_action(char **tokens, int count) {
    const char *cmd = tokens[0];

    /* 生成: gen */
    if (utils_str_eq(cmd, "gen")) {
        if (g_skip_gen) return 0;  /* 加载阶段跳过 gen (analyze 使用) */
        return commands_cmd_gen();
    }

    /* 分析: analyze - 统计代码行数、圈复杂度、代码重复率 */
    if (utils_str_eq(cmd, "analyze")) {
        return commands_cmd_analyze();
    }

    /* 更新: update - 扫描源码同步 .cboot
     * update 会重新生成 .cboot 文件，导致当前正在执行的脚本失效。
     * 返回 2 表示"脚本已失效"，parser 应停止读取后续行。 */
    if (utils_str_eq(cmd, "update")) {
        int rc = commands_cmd_update();
        /* 无论成功失败，脚本都已失效（文件可能被覆盖） */
        (void)rc;
        return 2;
    }

    /* 导入: im <path> - 仅API定义 */
    if (utils_str_eq(cmd, "im")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: im <.cboot 文件>\n");
            return -1;
        }
        return commands_cmd_im(tokens[1]);
    }

    /* 导入: in <path> - 完整项目作为子模块 */
    if (utils_str_eq(cmd, "in")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: in <.cboot 文件>\n");
            return -1;
        }
        return commands_cmd_in(tokens[1]);
    }

    /* 资源: res <file> */
    if (utils_str_eq(cmd, "res")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: res <资源文件>\n");
            return -1;
        }
        return commands_cmd_res(tokens[1]);
    }

    return PARSER_NOT_HANDLED;
}

static int parser_dispatch_script_line(char **tokens, int count) {
    const char *cmd = tokens[0];
    int rc;

    rc = parser_dispatch_build(tokens, count);
    if (rc != PARSER_NOT_HANDLED) return rc;

    rc = parser_dispatch_modify(tokens, count);
    if (rc != PARSER_NOT_HANDLED) return rc;

    rc = parser_dispatch_control(tokens, count);
    if (rc != PARSER_NOT_HANDLED) return rc;

    rc = parser_dispatch_action(tokens, count);
    if (rc != PARSER_NOT_HANDLED) return rc;

    /* .cboot 文件引用: <dir>/.cboot 或 .cboot
     * 引用失败（如文件不存在或子脚本有错误）视为非致命错误，仅警告并继续，
     * 以免阻断同一脚本中后续模块的加载。 */
    if (parser_is_cboot_ref(cmd)) {
        int ref_rc = parser_exec_cboot_ref(cmd);
        if (ref_rc != 0) {
            fprintf(stderr, "parser: 警告: .cboot 引用 '%s' 失败，已跳过\n", cmd);
        }
        return 0;
    }

    /* Unknown command */
    fprintf(stderr, "parser: 未知脚本命令: %s\n", cmd);
    return -1;
}

/* ------------------------------------------------------------------ */
/* parser_parse_cboot_script - read and execute a .cboot script              */
/* ------------------------------------------------------------------ */

/* 设置 g_script_dir 为 filename 所在目录；返回旧目录到 saved_dir */
static void parser_set_script_dir(const char *filename, char *saved_dir, size_t size) {
    strncpy(saved_dir, g_script_dir, size - 1);
    saved_dir[size - 1] = '\0';
    char dir[MAX_PATH_LEN];
    strncpy(dir, filename, MAX_PATH_LEN - 1);
    dir[MAX_PATH_LEN - 1] = '\0';
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        strcpy(g_script_dir, dir);
    } else {
        strcpy(g_script_dir, ".");
    }
}

/* 读取 heredoc 代码块（直到 EOF 行），并设置到当前域；返回 0=成功，-1=失败 */
static int parser_read_heredoc(FILE *f, int *line_no) {
    char code_buf[MAX_LINE_LEN * 64];
    code_buf[0] = '\0';
    int first = 1;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        (*line_no)++;
        size_t len2 = strlen(line);
        if (len2 > 0 && line[len2 - 1] == '\n') line[len2 - 1] = '\0';

        char *line_trimmed = utils_trim(line);
        if (utils_str_eq(line_trimmed, "EOF")) {
            if (!first) {
                size_t clen = strlen(code_buf);
                while (clen > 0 && code_buf[clen - 1] == '\n') code_buf[--clen] = '\0';
            }
            domain_domain_set_code(g_proj->current, code_buf);
            return 0;
        }
        if (line_trimmed[0] == '\0' && first) continue;
        if (!first) strncat(code_buf, "\n", sizeof(code_buf) - strlen(code_buf) - 1);
        strncat(code_buf, line, sizeof(code_buf) - strlen(code_buf) - 1);
        first = 0;
    }
    fprintf(stderr, "parser: code <<EOF 缺少代码内容\n");
    return -1;
}

/* 读取 tcode heredoc 代码块，作为测试用例添加到当前函数域 */
static int parser_read_tcode_heredoc(FILE *f, int *line_no) {
    char code_buf[MAX_LINE_LEN * 64];
    code_buf[0] = '\0';
    int first = 1;
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        (*line_no)++;
        size_t len2 = strlen(line);
        if (len2 > 0 && line[len2 - 1] == '\n') line[len2 - 1] = '\0';

        char *line_trimmed = utils_trim(line);
        if (utils_str_eq(line_trimmed, "EOF")) {
            if (!first) {
                size_t clen = strlen(code_buf);
                while (clen > 0 && code_buf[clen - 1] == '\n') code_buf[--clen] = '\0';
            }
            return commands_cmd_tcode(code_buf);
        }
        if (line_trimmed[0] == '\0' && first) continue;
        if (!first) strncat(code_buf, "\n", sizeof(code_buf) - strlen(code_buf) - 1);
        strncat(code_buf, line, sizeof(code_buf) - strlen(code_buf) - 1);
        first = 0;
    }
    fprintf(stderr, "parser: tcode <<EOF 缺少代码内容\n");
    return -1;
}

int parser_parse_cboot_script(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "parser: 无法打开脚本文件 '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    char saved_script_dir[MAX_PATH_LEN];
    parser_set_script_dir(filename, saved_script_dir, sizeof(saved_script_dir));

    char line[MAX_LINE_LEN];
    int line_no = 0;

    while (fgets(line, sizeof(line), f)) {
        line_no++;
        line[strcspn(line, "\n")] = '\0';

        char *trimmed = utils_trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        /* test 命令需要保留原始文本中的引号 (分词器会去除引号),
         * 所以在分词前直接处理, 传递 "test" 之后的原始文本 */
        if (strncmp(trimmed, "test", 4) == 0 && (trimmed[4] == ' ' || trimmed[4] == '\t')) {
            const char *args = trimmed + 4;
            while (*args == ' ' || *args == '\t') args++;
            if (commands_cmd_test(args) != 0) {
                fprintf(stderr, "parser: 第 %d 行执行失败\n", line_no);
                fclose(f);
                strcpy(g_script_dir, saved_script_dir);
                return -1;
            }
            continue;
        }

        int token_count = 0;
        char **tokens = tokenize(trimmed, &token_count);
        if (!tokens || token_count == 0) {
            utils_free_tokens(tokens, token_count);
            continue;
        }

        int ret = parser_dispatch_script_line(tokens, token_count);
        utils_free_tokens(tokens, token_count);

        if (ret == 1) {
            if (parser_read_heredoc(f, &line_no) != 0) {
                fclose(f);
                strcpy(g_script_dir, saved_script_dir);
                return -1;
            }
            continue;
        }
        if (ret == 3) {
            if (parser_read_tcode_heredoc(f, &line_no) != 0) {
                fclose(f);
                strcpy(g_script_dir, saved_script_dir);
                return -1;
            }
            continue;
        }
        if (ret == 2) {
            fclose(f);
            strcpy(g_script_dir, saved_script_dir);
            return 0;
        }
        if (ret != 0) {
            fprintf(stderr, "parser: 第 %d 行执行失败\n", line_no);
            fclose(f);
            strcpy(g_script_dir, saved_script_dir);
            return -1;
        }
    }

    fclose(f);
    strcpy(g_script_dir, saved_script_dir);
    return 0;
}
