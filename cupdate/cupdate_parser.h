/*
 * CBoot - C Project Bootstrapping Tool v0.5.0
 * C 语法分析器（parser）- 内部接口
 *
 * 仅暴露给 cupdate.c 使用。实现见 cupdate_parser.c。
 */

#ifndef CUPDATE_PARSER_H
#define CUPDATE_PARSER_H

#include "cupdate.h"
#include "cupdate_lexer.h"

/* ------------------------------------------------------------------ */
/* 解析器状态                                                          */
/* ------------------------------------------------------------------ */

typedef struct CuParser {
    CuLexer    *lex;        /* 词法分析器 */
    CUPResult  *result;     /* 解析结果收集 */
    const char *filename;   /* 文件名 */

    /* 预处理器状态：#if/#ifdef/#ifndef 嵌套深度，
     * 用于在 #if 0 块中跳过内容 */
    int         pp_skip_depth;  /* 当前处于跳过状态的 #if 嵌套层数 */
} CuParser;

/* ------------------------------------------------------------------ */
/* 接口                                                                */
/* ------------------------------------------------------------------ */

/* 解析整个翻译单元，结果写入 r。
 * source 为源码字符串，filename 用于错误信息。
 * 返回 0 表示成功（即使有语法错误也返回 0），
 * 返回 -1 表示致命错误。 */
int cup_parse(CUPResult *r, const char *source, const char *filename);

#endif /* CUPDATE_PARSER_H */
