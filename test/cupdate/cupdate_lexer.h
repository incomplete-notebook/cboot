/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * C 词法分析器（lexer）
 *
 * 设计参考 tinycc 的 tccpp.c，简化为单文件缓冲、无宏展开。
 * Token 编码：
 *   - 单字符 token：直接使用 ASCII 值（如 ';'、'{'、'(' 等）
 *   - 多字符运算符：使用 256+ 的枚举值
 *   - 关键字：使用 512+ 的枚举值
 *   - 标识符/数字/字符串/字符：使用专用枚举值
 */

#ifndef CUPDATE_LEXER_H
#define CUPDATE_LEXER_H

#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Token 编码                                                          */
/* ------------------------------------------------------------------ */

/* 多字符运算符（256+） */
typedef enum {
    CUP_TOK_OR_OR      = 256,   /* || */
    CUP_TOK_AND_AND    = 257,   /* && */
    CUP_TOK_EQ         = 258,   /* == */
    CUP_TOK_NEQ        = 259,   /* != */
    CUP_TOK_LE         = 260,   /* <= */
    CUP_TOK_GE         = 261,   /* >= */
    CUP_TOK_SHL        = 262,   /* << */
    CUP_TOK_SHR        = 263,   /* >> */
    CUP_TOK_PLUS_EQ    = 264,   /* += */
    CUP_TOK_MINUS_EQ   = 265,   /* -= */
    CUP_TOK_MUL_EQ     = 266,   /* *= */
    CUP_TOK_DIV_EQ     = 267,   /* /= */
    CUP_TOK_MOD_EQ     = 268,   /* %= */
    CUP_TOK_AND_EQ     = 269,   /* &= */
    CUP_TOK_OR_EQ      = 270,   /* |= */
    CUP_TOK_XOR_EQ     = 271,   /* ^= */
    CUP_TOK_SHL_EQ     = 272,   /* <<= */
    CUP_TOK_SHR_EQ     = 273,   /* >>= */
    CUP_TOK_INC        = 274,   /* ++ */
    CUP_TOK_DEC        = 275,   /* -- */
    CUP_TOK_PTR        = 276,   /* -> */
    CUP_TOK_ELLIPSIS   = 277,   /* ... */
    CUP_TOK_HASH_HASH  = 278,   /* ## */

    /* 字面量 */
    CUP_TOK_ID         = 300,   /* 标识符 */
    CUP_TOK_NUM        = 301,   /* 整数/浮点数字面量 */
    CUP_TOK_STR        = 302,   /* 字符串字面量 */
    CUP_TOK_CHAR       = 303,   /* 字符字面量 */

    /* 关键字 (512+) */
    CUP_TOK_KW_BASE    = 512,
    CUP_TOK_AUTO       = 512,
    CUP_TOK_BREAK      = 513,
    CUP_TOK_CASE       = 514,
    CUP_TOK_CHAR_KW    = 515,
    CUP_TOK_CONST      = 516,
    CUP_TOK_CONTINUE   = 517,
    CUP_TOK_DEFAULT    = 518,
    CUP_TOK_DO         = 519,
    CUP_TOK_DOUBLE     = 520,
    CUP_TOK_ELSE       = 521,
    CUP_TOK_ENUM       = 522,
    CUP_TOK_EXTERN     = 523,
    CUP_TOK_FLOAT      = 524,
    CUP_TOK_FOR        = 525,
    CUP_TOK_GOTO       = 526,
    CUP_TOK_IF         = 527,
    CUP_TOK_INLINE     = 528,
    CUP_TOK_INT        = 529,
    CUP_TOK_LONG       = 530,
    CUP_TOK_REGISTER   = 531,
    CUP_TOK_RESTRICT   = 532,
    CUP_TOK_RETURN     = 533,
    CUP_TOK_SHORT      = 534,
    CUP_TOK_SIGNED     = 535,
    CUP_TOK_SIZEOF     = 536,
    CUP_TOK_STATIC     = 537,
    CUP_TOK_STRUCT     = 538,
    CUP_TOK_SWITCH     = 539,
    CUP_TOK_TYPEDEF    = 540,
    CUP_TOK_UNION      = 541,
    CUP_TOK_UNSIGNED   = 542,
    CUP_TOK_VOID       = 543,
    CUP_TOK_VOLATILE   = 544,
    CUP_TOK_WHILE      = 545,
    CUP_TOK__BOOL      = 546,
    CUP_TOK__COMPLEX   = 547,
    CUP_TOK__IMAGINARY = 548,
    CUP_TOK__ATOMIC    = 549,
    CUP_TOK__STATIC_ASSERT = 550,
    CUP_TOK__GENERIC   = 551,
    CUP_TOK__NORETURN  = 552,
    CUP_TOK__ALIGNAS   = 553,
    CUP_TOK__ALIGNOF   = 554,

    CUP_TOK_EOF        = 768,
    CUP_TOK_ERROR      = 769,   /* 词法错误 */
    CUP_TOK_PP         = 770,   /* 行首 # （预处理器指令） */
    CUP_TOK_NEWLINE    = 771,   /* 换行（仅在预处理器模式下返回） */
} CuTokKind;

/* ------------------------------------------------------------------ */
/* Token 结构                                                          */
/* ------------------------------------------------------------------ */

typedef struct CuToken {
    int   kind;        /* CuTokKind */
    char *str;         /* ID/STR/CHAR/关键字: 字符串值；NUM: 原始字符串 */
    long  ival;        /* NUM: 整数值 */
    double fval;       /* NUM: 浮点值（如果是浮点） */
    int   is_float;    /* NUM: 1=浮点 */
    int   line;        /* 行号 */
    int   col;         /* 列号 */
    size_t start_pos;  /* token 在源码中的起始字节偏移 */
} CuToken;

/* ------------------------------------------------------------------ */
/* 词法分析器状态                                                       */
/* ------------------------------------------------------------------ */

/* 前向声明，避免循环包含 */
struct CUPResult;

typedef struct CuLexer {
    const char *src;        /* 源码字符串 */
    size_t      src_len;    /* 源码长度 */
    size_t      pos;        /* 当前读取位置 */
    int         line;       /* 当前行号 */
    int         col;        /* 当前列号 */
    const char *filename;   /* 文件名（用于错误信息） */

    /* 双 token 缓存：cur 是当前 token，peek 是 cur 之后的下一个 token。
     * 调用 cu_lex_next() 后，cur 变为原 peek，并扫描新的 peek。
     * 初始化后 cur 已是第一个 token。 */
    CuToken     cur;        /* 当前 token */
    CuToken     peek;       /* 下一个 token（始终有效，可能为 EOF） */
    int         has_peek;   /* peek 是否已填充 */

    /* 行首标志：用于识别 # 预处理器指令 */
    int         at_line_start;  /* 1=当前位于行首（仅空白和注释） */

    /* 错误收集：通过 CUPResult 接口添加错误 */
    struct CUPResult *result;
} CuLexer;

/* ------------------------------------------------------------------ */
/* 接口                                                                */
/* ------------------------------------------------------------------ */

/* 初始化词法分析器，传入源码字符串和文件名。
 * 初始化后 lex->cur 即为第一个 token。
 * result 用于收集词法错误。 */
void cu_lex_init(CuLexer *lex, const char *source, const char *filename,
                 struct CUPResult *result);

/* 释放词法分析器资源（不释放 errors 数组本身） */
void cu_lex_free(CuLexer *lex);

/* 推进一个 token：原 peek 变为新 cur，并扫描新 peek。
 * 返回新 cur 的 kind。 */
int cu_lex_next(CuLexer *lex);

/* 返回当前 cur 的 kind（不推进） */
int cu_lex_cur(CuLexer *lex);

/* 返回 peek 的 kind（cur 之后的下一个 token，不推进） */
int cu_lex_peek(CuLexer *lex);

/* 当前 token 是否为关键字，是则返回关键字的字符串名，否则返回 NULL */
const char *cu_tok_keyword_name(int kind);

/* token 是否为类型关键字（int/char/void/struct/...） */
int cu_tok_is_type_kw(int kind);

/* token 是否为存储类（static/extern/auto/register/typedef/inline/_Noreturn） */
int cu_tok_is_storage_kw(int kind);

/* token 是否为类型限定符（const/volatile/restrict/_Atomic） */
int cu_tok_is_qualifier_kw(int kind);

/* 报告词法错误 */
void cu_lex_error(CuLexer *lex, const char *msg);

#endif /* CUPDATE_LEXER_H */
