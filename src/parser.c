/*
 * CBoot - C Project Bootstrapping Tool v2.0
 * .cboot script parser (新规范 v2.0)
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Script line dispatch                                                */
/* ------------------------------------------------------------------ */

static int dispatch_script_line(char **tokens, int count) {
    const char *cmd = tokens[0];

    /* project <name> */
    if (str_eq(cmd, "project")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: project <名称>\n");
            return -1;
        }
        free(g_proj->name);
        g_proj->name = str_dup(tokens[1]);
        if (g_proj->root) {
            free(g_proj->root->name);
            g_proj->root->name = str_dup(tokens[1]);
        }
        return 0;
    }

    /* 建立域: mod <name> */
    if (str_eq(cmd, "mod")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: mod <名称>\n");
            return -1;
        }
        return cmd_mod(tokens[1]);
    }

    /* 建立域: struct <name> */
    if (str_eq(cmd, "struct")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: struct <名称>\n");
            return -1;
        }
        return cmd_struct(tokens[1]);
    }

    /* 建立域: type <name> */
    if (str_eq(cmd, "type")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: type <名称>\n");
            return -1;
        }
        return cmd_type(tokens[1]);
    }

    /* 建立域: def <name> */
    if (str_eq(cmd, "def")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: def <名称>\n");
            return -1;
        }
        return cmd_def(tokens[1]);
    }

    /* 带类型建立域: void <name> <type> */
    if (str_eq(cmd, "void")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: void <名称> <返回类型>\n");
            return -1;
        }
        return cmd_void(tokens[1], tokens[2]);
    }

    /* 带类型建立域: var <name> <type> */
    if (str_eq(cmd, "var")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: var <名称> <类型>\n");
            return -1;
        }
        return cmd_var(tokens[1], tokens[2]);
    }

    /* 带类型建立域: mem <name> <type> */
    if (str_eq(cmd, "mem")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: mem <名称> <类型>\n");
            return -1;
        }
        return cmd_mem(tokens[1], tokens[2]);
    }

    /* 枚举式def: enum <def1>,<def2>,... <start_num> */
    if (str_eq(cmd, "enum")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: enum <def1>,<def2>,... <start_num>\n");
            return -1;
        }
        return cmd_enum(tokens[1], tokens[2]);
    }

    /* 修改字段: cmt "text" */
    if (str_eq(cmd, "cmt")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: cmt \"文本\"\n");
            return -1;
        }
        /* Join all remaining tokens as comment text */
        char cmt_buf[MAX_LINE_LEN];
        cmt_buf[0] = '\0';
        for (int i = 1; i < count; i++) {
            if (i > 1) strcat(cmt_buf, " ");
            strcat(cmt_buf, tokens[i]);
        }
        strip_quotes(cmt_buf);
        return cmd_cmt(cmt_buf);
    }

    /* 修改字段: value <text> */
    if (str_eq(cmd, "value")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: value <值>\n");
            return -1;
        }
        return cmd_value(tokens[1]);
    }

    /* 修改字段: mode <text> */
    if (str_eq(cmd, "mode")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: mode <模式>\n");
            return -1;
        }
        return cmd_mode(tokens[1]);
    }

    /* 控制: cd <path> */
    if (str_eq(cmd, "cd")) {
        if (count < 2) {
            return cmd_cd(NULL);
        }
        return cmd_cd(tokens[1]);
    }

    /* 控制: rm <name> [-f] */
    if (str_eq(cmd, "rm")) {
        int force = 0;
        const char *name = NULL;
        for (int i = 1; i < count; i++) {
            if (str_eq(tokens[i], "-f"))
                force = 1;
            else
                name = tokens[i];
        }
        if (!name) {
            fprintf(stderr, "parser: 用法: rm <名称> [-f]\n");
            return -1;
        }
        return cmd_rm(name, force);
    }

    /* 查找: find <type> <pattern> [-a|-an] */
    if (str_eq(cmd, "find")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: find <类型> <字符> [-a|-an]\n");
            return -1;
        }
        int flags = 0;
        for (int i = 3; i < count; i++) {
            if (str_eq(tokens[i], "-a")) flags = 1;
            else if (str_startswith(tokens[i], "-a")) {
                flags = atoi(tokens[i] + 2);
            }
        }
        return cmd_find(tokens[1], tokens[2], flags);
    }

    /* 查看: ls [name] */
    if (str_eq(cmd, "ls")) {
        return cmd_ls(count >= 2 ? tokens[1] : NULL);
    }

    /* 移动: mv <src> <target> */
    if (str_eq(cmd, "mv")) {
        if (count < 3) {
            fprintf(stderr, "parser: 用法: mv <src> <target>\n");
            return -1;
        }
        return cmd_mv(tokens[1], tokens[2]);
    }

    /* 退出: exit */
    if (str_eq(cmd, "exit")) {
        g_running = 0;
        return 0;
    }

    /* 生成: gen */
    if (str_eq(cmd, "gen")) {
        return cmd_gen();
    }

    /* 导入: im <path> */
    if (str_eq(cmd, "im")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: im <.cboot 文件>\n");
            return -1;
        }
        return cmd_im(tokens[1]);
    }

    /* 资源: res <file> */
    if (str_eq(cmd, "res")) {
        if (count < 2) {
            fprintf(stderr, "parser: 用法: res <资源文件>\n");
            return -1;
        }
        return cmd_res(tokens[1]);
    }

    /* Unknown command */
    fprintf(stderr, "parser: 未知脚本命令: %s\n", cmd);
    return -1;
}

/* ------------------------------------------------------------------ */
/* parse_cboot_script - read and execute a .cboot script              */
/* ------------------------------------------------------------------ */

int parse_cboot_script(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "parser: 无法打开脚本文件 '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    char line[MAX_LINE_LEN];
    int line_no = 0;

    while (fgets(line, sizeof(line), f)) {
        line_no++;

        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        /* Trim whitespace */
        char *trimmed = trim(line);
        if (trimmed[0] == '\0') continue;

        /* Skip comments */
        if (trimmed[0] == '#') continue;

        /* Tokenize */
        int token_count = 0;
        char **tokens = tokenize(trimmed, &token_count);
        if (!tokens || token_count == 0) {
            free_tokens(tokens, token_count);
            continue;
        }

        /* Dispatch */
        int ret = dispatch_script_line(tokens, token_count);
        free_tokens(tokens, token_count);

        if (ret != 0) {
            fprintf(stderr, "parser: 第 %d 行执行失败\n", line_no);
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}