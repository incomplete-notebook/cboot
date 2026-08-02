/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * cboot update 模块 - 公共接口
 *
 * 功能：解析 C 源码，提取声明（函数/结构体/typedef/宏/变量），
 *       同步到 .cboot 描述文件，并报告语法错误。
 */

#ifndef CUPDATE_H
#define CUPDATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "domain/domain.h"

/* ------------------------------------------------------------------ */
/* 声明种类枚举                                                        */
/* ------------------------------------------------------------------ */

typedef enum {
    CUP_DECL_FUNCTION,   /* 函数声明或定义 */
    CUP_DECL_STRUCT,     /* 结构体定义 */
    CUP_DECL_ENUM,       /* 枚举定义 */
    CUP_DECL_TYPEDEF,    /* typedef 声明 */
    CUP_DECL_MACRO,      /* 宏定义 (#define) */
    CUP_DECL_VARIABLE,   /* 全局/静态变量声明 */
    CUP_DECL_INCLUDE,    /* #include 指令 */
    CUP_DECL_OTHER       /* 其他顶层声明（无法归类） */
} CUPDeclKind;

/* ------------------------------------------------------------------ */
/* 参数和成员结构                                                      */
/* ------------------------------------------------------------------ */

/* 函数参数 */
typedef struct CUParParam {
    char *type;   /* 参数类型字符串，如 "int", "const char*" */
    char *name;   /* 参数名，可能为 NULL（无名参数） */
} CUParParam;

/* 结构体/枚举成员 */
typedef struct CUParMember {
    char *type;   /* 成员类型（枚举时可为 NULL） */
    char *name;   /* 成员名 */
    char *value;  /* 位域宽度或初始值（可选） */
} CUParMember;

/* ------------------------------------------------------------------ */
/* 顶层声明结构                                                        */
/* ------------------------------------------------------------------ */

typedef struct CUPDecl {
    CUPDeclKind  kind;            /* 声明种类 */
    char        *name;            /* 声明名（函数名/结构体名/类型名/宏名/变量名） */
    char        *return_type;     /* 函数返回类型（仅 CUP_DECL_FUNCTION） */
    char        *base_type;       /* 变量/typedef 的基础类型；结构体的 tag 类型 */
    char        *call;            /* 调用约定：__cdecl, __stdcall, __fastcall 等 */
    char        *value;           /* 宏值 / typedef 别名原始字符串 / 变量初始值 */
    char        *body;            /* 函数体源码（仅 CUP_DECL_FUNCTION 且为定义时） */
    int          is_function_def; /* 1=是函数定义（含函数体），0=仅是声明 */
    int          is_static;       /* 1=static（私有），0=非 static */
    int          is_inline;       /* 1=inline 函数/结构体 */
    int          is_api;          /* 1=对外可见（非 static），0=static */
    int          line;            /* 声明所在行号（用于错误报告） */

    /* 函数参数列表 */
    CUParParam  *params;
    int          param_count;

    /* 结构体/枚举成员列表 */
    CUParMember *members;
    int          member_count;
} CUPDecl;

/* ------------------------------------------------------------------ */
/* 解析结果容器                                                        */
/* ------------------------------------------------------------------ */

typedef struct CUPResult {
    /* 解析出的顶层声明数组 */
    CUPDecl *decls;
    int      decl_count;
    int      decl_capacity;

    /* 语法错误（人类可读消息） */
    char   **errors;
    int      error_count;
    int      error_capacity;

    /* 警告（人类可读消息） */
    char   **warnings;
    int      warning_count;
    int      warning_capacity;
} CUPResult;

/* ------------------------------------------------------------------ */
/* 公共接口                                                            */
/* ------------------------------------------------------------------ */

/* 解析整个项目：递归所有 src 模块，解析 .c 同步到 domain 树。
 * 返回 0=成功，-1=有错误或致命失败。
 * error_count_out, warning_count_out 可选（为 NULL 则不输出） */
int cupdate_run_project(Project *proj, int *error_count_out, int *warning_count_out);

/* 解析单个 C 源文件，结果写入 r（调用者负责 init/free）。
 * 返回 0=成功（即使有语法错误也返回 0，错误在 r->errors 中），
 * 返回 -1=致命错误（内存分配失败等）。 */
int cupdate_parse_source(const char *source, const char *filename, CUPResult *r);

/* 初始化 / 释放结果容器 */
void cupdate_result_init(CUPResult *r);
void cupdate_result_free(CUPResult *r);

/* 向结果容器追加错误 / 警告（filename+line 形式的消息已拼装好） */
void cupdate_result_add_error(CUPResult *r, const char *msg, int line);
void cupdate_result_add_warning(CUPResult *r, const char *msg, int line);

/* 在结果容器中创建一个新的空声明，返回其指针（已零初始化） */
CUPDecl *cupdate_result_add_decl(CUPResult *r);

/* 释放 CUParParam 数组（循环 free type/name + free 数组头）。
 * cupdate.c 和 cupdate_parser.c 共用。 */
void cup_free_param_array(CUParParam *params, int count);

/* 单个模块级别的 update：解析模块对应的 .c 文件并同步。
 * mod_dir 是模块目录（含 .c 文件的目录）。
 * 返回 0=成功，-1=有错误。 */
int cupdate_run_module(ModuleDomain *mod, const char *mod_dir);

#endif /* CUPDATE_H */
