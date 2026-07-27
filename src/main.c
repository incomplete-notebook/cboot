/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * Main entry point (新规范 v2.0)
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/* ------------------------------------------------------------------ */
/* Global state                                                       */
/* ------------------------------------------------------------------ */

Project *g_proj   = NULL;
RunMode  g_mode   = MODE_INTERACTIVE;
int      g_force  = 0;
int      g_running = 1;

/* ------------------------------------------------------------------ */
/* Forward declarations                                               */
/* ------------------------------------------------------------------ */

static void print_usage(const char *prog);
static void repl_loop(void);
static int  dispatch_command(char **tokens, int count);
static int  detect_fine_tune_mode(void);

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv) {
    const char *proj_name = NULL;
    const char *to_cboot_dir = NULL;

    /* 解析命令行参数 */
    for (int i = 1; i < argc; i++) {
        if (str_eq(argv[i], "-f") || str_eq(argv[i], "--force")) {
            g_force = 1;
        } else if (str_eq(argv[i], "-h") || str_eq(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        } else if (str_eq(argv[i], "-to_cboot")) {
            g_mode = MODE_TO_CBOOT;
            if (i + 1 < argc) {
                to_cboot_dir = argv[++i];
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "cboot: 未知选项 '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            proj_name = argv[i];
        }
    }

    /* to_cboot 模式 */
    if (g_mode == MODE_TO_CBOOT) {
        if (!to_cboot_dir) {
            fprintf(stderr, "cboot: -to_cboot 需要指定目录\n");
            fprintf(stderr, "用法: cboot -to_cboot <目录>\n");
            return 1;
        }
        printf("CBoot v%s - to_cboot 模式\n", CBOOT_VERSION);
        printf("目标目录: %s\n", to_cboot_dir);
        printf("(to_cboot 功能尚未实现)\n");
        return 0;
    }

    /* 批处理模式: 无项目名，读取 .cboot */
    if (!proj_name) {
        g_mode = MODE_BATCH;

        /* 检测微调模式 */
        if (file_exists(".cboot") && detect_fine_tune_mode()) {
            g_mode = MODE_FINE_TUNE;
            printf("CBoot v%s - 微调模式\n", CBOOT_VERSION);
            printf("检测到已生成的项目，进入微调模式。\n");
            g_proj = project_new("cboot_project");
            if (parse_cboot_script(".cboot") != 0) {
                fprintf(stderr, "cboot: 脚本执行失败\n");
                return 1;
            }
            /* 进入 REPL 进行微调 */
            repl_loop();
            project_free(g_proj);
            return 0;
        }

        if (!file_exists(".cboot")) {
            fprintf(stderr, "cboot: 未找到 .cboot 脚本，且未指定项目名\n");
            print_usage(argv[0]);
            return 1;
        }
        g_proj = project_new("cboot_project");
        printf("CBoot v%s - 批处理模式\n", CBOOT_VERSION);
        if (parse_cboot_script(".cboot") != 0) {
            fprintf(stderr, "cboot: 脚本执行失败\n");
            return 1;
        }
        project_free(g_proj);
        return 0;
    }

    /* 交互模式 */
    g_mode = MODE_INTERACTIVE;

    /* 校验项目名 */
    if (!is_valid_identifier(proj_name)) {
        fprintf(stderr, "cboot: 无效的项目名 '%s'（仅允许字母、数字、下划线）\n", proj_name);
        return 1;
    }

    /* 处理 -f 强制重建 */
    if (g_force && file_exists(proj_name)) {
        char cmd[MAX_PATH_LEN];
        snprintf(cmd, sizeof(cmd), "rm -rf %s", proj_name);
        system(cmd);
    }

    if (file_exists(proj_name) && !g_force) {
        fprintf(stderr, "cboot: 目录 '%s' 已存在。使用 -f 强制重建。\n", proj_name);
        return 1;
    }

    /* 创建项目目录 */
    ensure_dir(proj_name);
    if (chdir(proj_name) != 0) {
        perror("cboot: chdir");
        return 1;
    }

    /* 初始化项目 */
    g_proj = project_new(proj_name);

    printf("CBoot v%s - 交互模式\n", CBOOT_VERSION);
    printf("项目 '%s' 已创建。输入命令开始构建，输入 help 查看帮助。\n\n", proj_name);

    /* 进入 REPL */
    repl_loop();

    /* 清理 */
    project_free(g_proj);
    return 0;
}

/* ------------------------------------------------------------------ */
/* detect_fine_tune_mode - check if .cboot project has been generated  */
/* ------------------------------------------------------------------ */

static int detect_fine_tune_mode(void) {
    /* 检测是否已生成项目：检查 CMakeLists.txt 和 main.c */
    if (file_exists("CMakeLists.txt")) {
        if (file_exists("main.c")) {
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Usage                                                              */
/* ------------------------------------------------------------------ */

static void print_usage(const char *prog) {
    printf("CBoot v%s - C 项目引导工具\n\n", CBOOT_VERSION);
    printf("用法:\n");
    printf("  %s <项目名>          交互模式，创建项目并进入 REPL\n", prog);
    printf("  %s <项目名> -f       交互模式，强制重建项目\n", prog);
    printf("  %s                   批处理模式，读取 .cboot 脚本\n", prog);
    printf("  %s -to_cboot <目录>  反向工程：将 C 项目转为 .cboot\n", prog);
    printf("  %s -h                显示帮助\n", prog);
    printf("\n交互模式命令:\n");
    printf("  建立域:\n");
    printf("    mod <名称>             创建模块\n");
    printf("    struct <名称>          创建结构体\n");
    printf("    type <名称>            创建类型\n");
    printf("    def <名称>             创建宏\n");
    printf("    void <名称> <类型>     创建函数\n");
    printf("    var <名称> <类型>      创建变量\n");
    printf("    mem <名称> <类型>      创建成员/参数\n");
    printf("    enum <defs> <start>    枚举式创建宏\n");
    printf("  修改字段:\n");
    printf("    cmt \"文本\"             设置注释\n");
    printf("    value <值>             设置值\n");
    printf("    mode <模式>            设置模式 (internal/external/api/normal/...)\n");
    printf("    cmode <模式>           设置编译器模式 (exe/sl/dl/normal)\n");
    printf("  控制:\n");
    printf("    cd <路径>              导航域树\n");
    printf("    rm <名称> [-f]         删除子域\n");
    printf("    mv <src> <target>      移动域\n");
    printf("    find <类型> <字符>     搜索域\n");
    printf("    ls [名称]              查看域内容\n");
    printf("  其他:\n");
    printf("    gen                    生成代码\n");
    printf("    im <.cboot 文件>       导入脚本\n");
    printf("    res <文件>             添加资源\n");
    printf("    exit                   退出\n");
}

/* ------------------------------------------------------------------ */
/* Tab completion helpers                                               */
/* ------------------------------------------------------------------ */

/* All command names for tab completion */
static const char *g_cmd_names[] = {
    "mod", "struct", "type", "def", "void", "var", "mem", "enum",
    "cmt", "value", "mode", "cmode",
    "cd", "rm", "mv", "find", "ls",
    "gen", "im", "in", "res", "exit", "help", "?",
    NULL
};

/* Domain type names for find command */
static const char *g_domain_types[] = {
    "mod", "void", "struct", "type", "def", "var", "mem",
    NULL
};

/* Mode values for mode command */
static const char *g_mode_values[] = {
    "internal", "external", "api", "normal", "static",
    "rename", "struct", "api rename", "api struct",
    NULL
};

/* Compiler mode values for cmode command */
static const char *g_cmode_values[] = {
    "exe", "sl", "dl", "normal",
    NULL
};

/*
 * try_complete - given a prefix, find all matching strings.
 * Returns number of matches; fills matches[] with pointers.
 */
static int try_complete(const char *prefix, const char **candidates, int max_matches,
                        const char **matches)
{
    int count = 0;
    int plen = (int)strlen(prefix);
    for (int i = 0; candidates[i] && count < max_matches; i++) {
        if (strncmp(candidates[i], prefix, plen) == 0) {
            matches[count++] = candidates[i];
        }
    }
    return count;
}

/*
 * common_prefix_len - find the length of the longest common prefix
 * among the given strings.
 */
static int common_prefix_len(const char **strs, int count)
{
    if (count <= 0) return 0;
    int len = (int)strlen(strs[0]);
    for (int i = 1; i < count; i++) {
        int j = 0;
        while (j < len && strs[0][j] == strs[i][j]) j++;
        len = j;
    }
    return len;
}

/*
 * do_tab_complete - perform tab completion on the current line.
 * Modifies line and updates pos. Returns 1 if display was refreshed.
 */
static int do_tab_complete(char *line, int *pos, const char *prompt)
{
    /* Make a working copy of the line up to cursor */
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, *pos);
    buf[*pos] = '\0';

    /* Tokenize what's been typed so far */
    int tc = 0;
    char **tokens = tokenize(buf, &tc);

    const char *matches[128];
    int match_count = 0;

    if (tc == 0 || (tc == 1 && buf[*pos - 1] != ' ')) {
        /* Completing first token (command name) */
        const char *prefix = (tc > 0) ? tokens[0] : "";
        match_count = try_complete(prefix, g_cmd_names, 128, matches);
    } else {
        /* Completing second+ token - context-sensitive */
        const char *cmd = tokens[0];
        const char *partial = (tc > 1) ? tokens[tc - 1] : "";
        int is_last_space = (buf[*pos - 1] == ' ');

        if (str_eq(cmd, "cd") || str_eq(cmd, "rm") || str_eq(cmd, "ls")) {
            /* Complete with child domain names */
            int plen = (int)strlen(partial);
            Domain *cur = g_proj->current;
            for (int i = 0; i < cur->child_count && match_count < 128; i++) {
                if (strncmp(cur->children[i]->name, partial, plen) == 0) {
                    matches[match_count++] = cur->children[i]->name;
                }
            }
        } else if (str_eq(cmd, "find") && tc == 2 && !is_last_space) {
            /* Complete domain type for find command */
            match_count = try_complete(partial, g_domain_types, 128, matches);
        } else if (str_eq(cmd, "mode")) {
            /* Complete mode values */
            match_count = try_complete(partial, g_mode_values, 128, matches);
        } else if (str_eq(cmd, "cmode")) {
            /* Complete compiler mode values */
            match_count = try_complete(partial, g_cmode_values, 128, matches);
        } else {
            /* Default: complete with child domain names */
            int plen = (int)strlen(partial);
            Domain *cur = g_proj->current;
            for (int i = 0; i < cur->child_count && match_count < 128; i++) {
                if (strncmp(cur->children[i]->name, partial, plen) == 0) {
                    matches[match_count++] = cur->children[i]->name;
                }
            }
        }
    }

    free_tokens(tokens, tc);

    if (match_count == 0) {
        /* No match - beep */
        printf("\a");
        fflush(stdout);
        return 0;
    }

    if (match_count == 1) {
        /* Single match - complete it */
        /* Find where the current partial token starts */
        char *last_space = strrchr(buf, ' ');
        int base_len;
        if (last_space)
            base_len = (int)(last_space - buf) + 1;
        else
            base_len = 0;

        const char *completion = matches[0];
        const char *suffix = completion + (*pos - base_len);
        int suffix_len = (int)strlen(suffix);

        /* Append suffix + space */
        if (*pos + suffix_len + 1 < MAX_LINE_LEN) {
            memcpy(line + *pos, suffix, suffix_len);
            line[*pos + suffix_len] = ' ';
            line[*pos + suffix_len + 1] = '\0';
            *pos = *pos + suffix_len + 1;
        }
        /* Redisplay */
        printf("\r\033[K%s%s", prompt, line);
        fflush(stdout);
        return 1;
    }

    /* Multiple matches - complete to common prefix */
    char *last_space = strrchr(buf, ' ');
    int base_len;
    if (last_space)
        base_len = (int)(last_space - buf) + 1;
    else
        base_len = 0;

    int cplen = common_prefix_len(matches, match_count);
    const char *first = matches[0];
    const char *suffix = first + (*pos - base_len);
    int new_cplen = cplen - (*pos - base_len);
    if (new_cplen < 0) new_cplen = 0;

    if (new_cplen > 0 && *pos + new_cplen < MAX_LINE_LEN) {
        memcpy(line + *pos, suffix, new_cplen);
        line[*pos + new_cplen] = '\0';
        *pos = *pos + new_cplen;
    }

    /* Show the common prefix completion, then list matches */
    printf("\r\033[K%s%s\n", prompt, line);
    for (int i = 0; i < match_count; i++) {
        printf("%s  ", matches[i]);
    }
    printf("\n%s%s", prompt, line);
    fflush(stdout);
    return 1;
}

/* ------------------------------------------------------------------ */
/* REPL loop                                                          */
/* ------------------------------------------------------------------ */

#define CBOOT_HISTORY_MAX 100

static void repl_loop(void) {
    char line[MAX_LINE_LEN];
    char saved_line[MAX_LINE_LEN]; /* saves in-progress line when browsing history */
    char prompt[MAX_PATH_LEN];
    char *history[CBOOT_HISTORY_MAX] = {0};
    int hist_count = 0;
    int hist_pos = 0;
    int hist_browsing = 0; /* whether we are browsing history */

    /* 保存并设置终端为 raw 模式 */
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* 启用 bracketed paste mode */
    printf("\033[?2004h");
    fflush(stdout);

    while (g_running) {
        /* 构建提示符 */
        char *path = domain_get_path(g_proj->current);
        snprintf(prompt, sizeof(prompt), "CBoot%s> ", path);
        free(path);

        /* 显示提示符 */
        printf("%s", prompt);
        fflush(stdout);

        /* 逐字符读取输入 */
        int pos = 0;
        int esc_state = 0;   /* 0=normal, 1=saw ESC, 2=saw ESC[, 3=saw ESC[? (bracketed paste) */
        line[0] = '\0';

        while (1) {
            char c;
            if (read(STDIN_FILENO, &c, 1) != 1) {
                g_running = 0;
                break;
            }

            /* 处理转义序列 */
            if (esc_state == 0 && c == '\033') {
                esc_state = 1;
                continue;
            }
            if (esc_state == 1) {
                if (c == '[') {
                    esc_state = 2;
                    continue;
                }
                /* Lone ESC */
                esc_state = 0;
                continue;
            }
            if (esc_state == 2) {
                if (c == 'A') {
                    /* 上箭头 - 命令回滚 */
                    if (hist_count > 0) {
                        if (!hist_browsing) {
                            /* Save current in-progress line */
                            strcpy(saved_line, line);
                            hist_browsing = 1;
                            hist_pos = hist_count; /* will be decremented below */
                        }
                        if (hist_pos > 0) {
                            hist_pos--;
                            strcpy(line, history[hist_pos]);
                            printf("\r\033[K%s%s", prompt, line);
                            fflush(stdout);
                            pos = (int)strlen(line);
                        }
                    }
                    esc_state = 0;
                    continue;
                } else if (c == 'B') {
                    /* 下箭头 - 命令回滚 */
                    if (hist_browsing) {
                        if (hist_pos < hist_count - 1) {
                            hist_pos++;
                            strcpy(line, history[hist_pos]);
                        } else {
                            /* Past the last history entry - restore saved line */
                            hist_pos = hist_count;
                            strcpy(line, saved_line);
                            hist_browsing = 0;
                        }
                        printf("\r\033[K%s%s", prompt, line);
                        fflush(stdout);
                        pos = (int)strlen(line);
                    }
                    esc_state = 0;
                    continue;
                } else if (c == 'C') {
                    /* 右箭头 - 忽略，清除esc状态 */
                    esc_state = 0;
                    continue;
                } else if (c == 'D') {
                    /* 左箭头 - 忽略，清除esc状态 */
                    esc_state = 0;
                    continue;
                } else if (c == '?') {
                    /* Bracketed paste start prefix: ESC[? */
                    esc_state = 3;
                    continue;
                } else if (c >= '0' && c <= '9') {
                    /* Extended escape sequence like ESC[1~ or ESC[200~ */
                    /* Read the rest until '~' or letter */
                    while (1) {
                        if (read(STDIN_FILENO, &c, 1) != 1) break;
                        if (c == '~' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                            break;
                    }
                    /* Bracketed paste ESC[200~/ESC[201~ and others are consumed silently */
                    esc_state = 0;
                    continue;
                } else {
                    /* Unknown ESC[ sequence */
                    esc_state = 0;
                    continue;
                }
            }
            if (esc_state == 3) {
                /* After ESC[?, consume digits then ~ (e.g. ESC[?2004h reply) */
                if (c >= '0' && c <= '9') {
                    while (1) {
                        if (read(STDIN_FILENO, &c, 1) != 1) break;
                        if (c == '~' || c == 'h' || c == 'l') break;
                    }
                }
                esc_state = 0;
                continue;
            }

            /* Tab - command completion */
            if (c == '\t') {
                if (pos > 0) {
                    do_tab_complete(line, &pos, prompt);
                }
                continue;
            }

            /* Enter */
            if (c == '\n' || c == '\r') {
                line[pos] = '\0';
                printf("\n");
                break;
            }

            /* Ctrl+C - clear line */
            if (c == 3) {
                line[0] = '\0';
                pos = 0;
                printf("^C\n%s", prompt);
                fflush(stdout);
                continue;
            }

            /* Ctrl+D - exit on empty line */
            if (c == 4) {
                if (pos == 0) {
                    printf("\n");
                    g_running = 0;
                    break;
                }
                continue;
            }

            /* Ctrl+L - clear screen */
            if (c == 12) {
                printf("\033[2J\033[H%s%s", prompt, line);
                fflush(stdout);
                continue;
            }

            /* Backspace */
            if (c == 127 || c == '\b') {
                if (pos > 0) {
                    pos--;
                    line[pos] = '\0';
                    printf("\b \b");
                    fflush(stdout);
                }
                continue;
            }

            /* 普通字符 (including pasted text) */
            if (pos < MAX_LINE_LEN - 1 && c >= 32) {
                line[pos++] = c;
                line[pos] = '\0';
                putchar(c);
                fflush(stdout);
            }
        }

        if (!g_running) break;

        /* Reset history browsing state */
        hist_browsing = 0;

        /* 跳过空行 */
        char *trimmed = trim(line);
        if (trimmed[0] == '\0') continue;

        /* 添加到历史 */
        if (hist_count == 0 || strcmp(line, history[hist_count - 1]) != 0) {
            if (hist_count < CBOOT_HISTORY_MAX) {
                history[hist_count] = strdup(line);
                hist_count++;
            } else {
                free(history[0]);
                for (int i = 0; i < CBOOT_HISTORY_MAX - 1; i++)
                    history[i] = history[i + 1];
                history[CBOOT_HISTORY_MAX - 1] = strdup(line);
            }
        }
        hist_pos = hist_count;

        /* 分词并分发 */
        int token_count = 0;
        char **tokens = tokenize(trimmed, &token_count);
        if (!tokens || token_count == 0) {
            free_tokens(tokens, token_count);
            continue;
        }

        dispatch_command(tokens, token_count);
        free_tokens(tokens, token_count);
    }

    /* 禁用 bracketed paste mode 并恢复终端 */
    printf("\033[?2004l");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    /* 释放历史 */
    for (int i = 0; i < hist_count; i++)
        free(history[i]);
}

/* ------------------------------------------------------------------ */
/* Command dispatcher (新规范 v2.0)                                     */
/* ------------------------------------------------------------------ */

static int dispatch_command(char **tokens, int count) {
    const char *cmd = tokens[0];

    /* 帮助 */
    if (str_eq(cmd, "help") || str_eq(cmd, "?")) {
        print_usage("cboot");
        return 0;
    }

    /* 建立域: <op> <name> */
    if (str_eq(cmd, "mod")) {
        if (count < 2) { printf("用法: mod <名称>\n"); return -1; }
        return cmd_mod(tokens[1]);
    }
    if (str_eq(cmd, "struct")) {
        if (count < 2) { printf("用法: struct <名称>\n"); return -1; }
        return cmd_struct(tokens[1]);
    }
    if (str_eq(cmd, "type")) {
        if (count < 2) { printf("用法: type <名称>\n"); return -1; }
        return cmd_type(tokens[1]);
    }
    if (str_eq(cmd, "def")) {
        if (count < 2) { printf("用法: def <名称>\n"); return -1; }
        return cmd_def(tokens[1]);
    }

    /* 带类型建立域: <op> <name> <type> */
    if (str_eq(cmd, "void")) {
        if (count < 3) { printf("用法: void <名称> <返回类型>\n"); return -1; }
        return cmd_void(tokens[1], tokens[2]);
    }
    if (str_eq(cmd, "var")) {
        if (count < 3) { printf("用法: var <名称> <类型>\n"); return -1; }
        return cmd_var(tokens[1], tokens[2]);
    }
    if (str_eq(cmd, "mem")) {
        if (count < 3) { printf("用法: mem <名称> <类型>\n"); return -1; }
        return cmd_mem(tokens[1], tokens[2]);
    }

    /* 枚举式def */
    if (str_eq(cmd, "enum")) {
        if (count < 3) { printf("用法: enum <def1>,<def2>,... <start_num>\n"); return -1; }
        return cmd_enum(tokens[1], tokens[2]);
    }

    /* 修改字段 */
    if (str_eq(cmd, "cmt")) {
        if (count < 2) { printf("用法: cmt \"文本\"\n"); return -1; }
        /* Join rest */
        char cmt_buf[MAX_LINE_LEN] = {0};
        for (int i = 1; i < count; i++) {
            if (i > 1) strcat(cmt_buf, " ");
            strcat(cmt_buf, tokens[i]);
        }
        strip_quotes(cmt_buf);
        return cmd_cmt(cmt_buf);
    }
    if (str_eq(cmd, "value")) {
        if (count < 2) { printf("用法: value <值>\n"); return -1; }
        return cmd_value(tokens[1]);
    }
    if (str_eq(cmd, "mode")) {
        if (count < 2) { printf("用法: mode <模式>\n"); return -1; }
        char mode_buf[MAX_LINE_LEN] = {0};
        for (int i = 1; i < count; i++) {
            if (i > 1) strcat(mode_buf, " ");
            strcat(mode_buf, tokens[i]);
        }
        return cmd_mode(mode_buf);
    }
    if (str_eq(cmd, "cmode")) {
        if (count < 2) { printf("用法: cmode <exe|sl|dl|normal>\n"); return -1; }
        return cmd_cmode(tokens[1]);
    }

    /* 控制 */
    if (str_eq(cmd, "cd")) {
        return cmd_cd(count >= 2 ? tokens[1] : NULL);
    }
    if (str_eq(cmd, "rm")) {
        int force = 0;
        const char *name = NULL;
        for (int i = 1; i < count; i++) {
            if (str_eq(tokens[i], "-f")) force = 1;
            else name = tokens[i];
        }
        if (!name) { printf("用法: rm <名称> [-f]\n"); return -1; }
        return cmd_rm(name, force);
    }

    /* 查找 */
    if (str_eq(cmd, "find")) {
        if (count < 3) {
            printf("用法: find <类型> <字符> [-a|-an]\n");
            return -1;
        }
        int flags = 0;
        for (int i = 3; i < count; i++) {
            if (str_eq(tokens[i], "-a")) flags = 1;
            else if (str_startswith(tokens[i], "-a")) flags = atoi(tokens[i] + 2);
        }
        return cmd_find(tokens[1], tokens[2], flags);
    }

    /* 查看 */
    if (str_eq(cmd, "ls")) {
        return cmd_ls(count >= 2 ? tokens[1] : NULL);
    }

    /* 移动 */
    if (str_eq(cmd, "mv")) {
        if (count < 3) { printf("用法: mv <src> <target>\n"); return -1; }
        return cmd_mv(tokens[1], tokens[2]);
    }

    /* 退出 */
    if (str_eq(cmd, "exit")) {
        return cmd_exit();
    }

    /* 生成 */
    if (str_eq(cmd, "gen")) {
        return cmd_gen();
    }

    /* 导入: im - 仅API定义 */
    if (str_eq(cmd, "im")) {
        if (count < 2) { printf("用法: im <.cboot 文件>\n"); return -1; }
        return cmd_im(tokens[1]);
    }

    /* 导入: in - 完整项目作为子模块 */
    if (str_eq(cmd, "in")) {
        if (count < 2) { printf("用法: in <.cboot 文件>\n"); return -1; }
        return cmd_in(tokens[1]);
    }

    /* 资源 */
    if (str_eq(cmd, "res")) {
        if (count < 2) { printf("用法: res <资源文件>\n"); return -1; }
        return cmd_res(tokens[1]);
    }

    /* .cboot 文件引用: <dir>/.cboot 或 .cboot */
    if (try_cboot_ref(cmd)) {
        return 0;
    }

    printf("未知命令: %s (输入 help 查看帮助)\n", cmd);
    return -1;
}