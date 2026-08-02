/*
 * CBoot - C Project Bootstrapping Tool v1.1.0
 *
 * Copyright (c) 2026 CBoot Contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#ifndef CBOOT_H
#define CBOOT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "domain/domain.h"
#include "typecheck/typecheck.h"
#include "cupdate/cupdate.h"

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */

#define CBOOT_VERSION      "1.1.0"
#define MAX_LINE_LEN       4096
#define MAX_PATH_LEN       1024
#define MAX_NAME_LEN       256
#define MAX_TOKEN_COUNT    64
#define MAX_CHILDREN       256
#define MAX_MACROS         256

/* ------------------------------------------------------------------ */
/* Global state                                                       */
/* ------------------------------------------------------------------ */

extern Project *g_proj;
extern RunMode  g_mode;
extern int      g_force;       /* -f flag */
extern int      g_running;     /* REPL running flag */
extern int      g_skip_gen;    /* 加载 .cboot 时跳过 gen 命令 (analyze 使用) */
extern char     g_script_dir[]; /* 当前脚本文件所在目录，用于 .cboot 引用解析 */

/* ------------------------------------------------------------------ */
/* utils.c - Utility functions                                          */
/* ------------------------------------------------------------------ */

char  **tokenize(const char *line, int *count);
void    utils_free_tokens(char **tokens, int count);
char   *utils_trim(char *str);
char   *utils_str_dup(const char *str);
int     utils_str_eq(const char *a, const char *b);
int     utils_str_startswith(const char *str, const char *prefix);
int     utils_parse_c_decl(const char *decl, char *type_out, int type_size,
                     char *name_out, int name_size);
char   *extract_base_type(const char *type_decl);
char   *extract_type_from_decl(const char *decl);
char   *extract_name_from_decl(const char *decl);
char  **utils_tokenize(const char *line, int *count);
char   *utils_extract_base_type(const char *type_decl);
char   *utils_extract_type_from_decl(const char *decl);
char   *utils_extract_name_from_decl(const char *decl);
int     utils_is_valid_identifier(const char *name);
void    utils_ensure_dir(const char *path);
int     utils_file_exists(const char *path);
void    utils_strip_quotes(char *str);

/* ------------------------------------------------------------------ */
/* commands.c - Command handlers (新规范)                                */
/* ------------------------------------------------------------------ */

/* 建立域: <op> <name> */
int commands_cmd_mod(const char *name);
int commands_cmd_struct(const char *name);
int commands_cmd_type(const char *name);
int commands_cmd_def(const char *name);

/* 带有type的建立域: <op> <name> <type> */
int commands_cmd_void(const char *name, const char *return_type);
int commands_cmd_var(const char *name, const char *type);
int commands_cmd_mem(const char *name, const char *type);

/* 枚举式建立def域: enum <def1>,<def2>,... <start_num> */
int commands_cmd_enum(const char *defs, const char *start_num);

/* 修改字段: <op> <value> */
int commands_cmd_cmt(const char *text);
int commands_cmd_value(const char *text);
int commands_cmd_call(const char *call_conv);
int commands_cmd_mode(const char *text);
int commands_cmd_cmode(const char *text);  /* 设置编译器模式: exe/sl/dl/normal */
int commands_cmd_test(const char *buf);    /* 添加测试用例: test <输入> => <预期> */
int commands_cmd_tcode(const char *code);  /* 添加代码测试用例: tcode <<EOF */

/* 控制: <op> <域> */
int commands_cmd_cd(const char *path);
int commands_cmd_rm(const char *name, int force);

/* 查找: find <type> <pattern> [-a|-an] */
int commands_cmd_find(const char *type_filter, const char *pattern, int flags);

/* 查看: ls [domain] */
int commands_cmd_ls(const char *name);

/* 移动: mv <src> <target> */
int commands_cmd_mv(const char *src, const char *target);

/* 退出: exit */
int commands_cmd_exit(void);

/* 生成: gen */
int commands_cmd_gen(void);

/* 更新: update - 扫描源码同步 .cboot */
int commands_cmd_update(void);

/* 分析: analyze - 统计有效代码行数、圈复杂度、代码重复率 */
int commands_cmd_analyze(void);

/* 微调: adjust - 先update再进入交互式REPL调整 */
int commands_cmd_adjust(void);

/* 导入: im <.cboot file> - 仅导入API定义，记录依赖链（项目内） */
int commands_cmd_im(const char *path);

/* 导入: in <.cboot file> - 复制整个项目作为子模块 */
int commands_cmd_in(const char *path);

/* 资源: res <file> */
int commands_cmd_res(const char *file_path);

/* ------------------------------------------------------------------ */
/* parser.c - .cboot script parser                                      */
/* ------------------------------------------------------------------ */

int parser_parse_cboot_script(const char *filename);

/* 检查 token 是否为 .cboot 文件引用，如果是则执行它
 * 返回 1 表示是引用且已执行，0 表示不是引用 */
int parser_try_cboot_ref(const char *token);

/* ------------------------------------------------------------------ */
/* generator.c - Code generator                                         */
/* ------------------------------------------------------------------ */

int generator_generate_project(Project *proj);

/* 仅重新生成 .cboot 文件（不覆盖 .c/.h/CMake） */
int generator_generate_cboot_only(Project *proj);

/* ------------------------------------------------------------------ */
/* docgen.c - Documentation generator (4种.md)                          */
/* ------------------------------------------------------------------ */

int  docgen_generate_docs(Project *proj, const char *output_dir);
void docgen_generate_module_docs(Domain *mod, const char *dir);

#endif /* CBOOT_H */
