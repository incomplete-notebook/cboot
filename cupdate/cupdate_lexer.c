/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * C 词法分析器实现
 *
 * 参考 tinycc tccpp.c 的字符扫描逻辑，简化为：
 *   - 单缓冲区，无 macro_ptr 嵌套
 *   - 不展开宏，但保留 # 指令的原始 token 流
 *   - 在行首（仅空白和注释之后）的 # 视为预处理器指令开始
 */

#include "cupdate_lexer.h"
#include "cupdate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* 关键字表                                                            */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *name;
    int         kind;
} KwEntry;

static const KwEntry g_keywords[] = {
    {"auto",        CUP_TOK_AUTO},
    {"break",       CUP_TOK_BREAK},
    {"case",        CUP_TOK_CASE},
    {"char",        CUP_TOK_CHAR_KW},
    {"const",       CUP_TOK_CONST},
    {"continue",    CUP_TOK_CONTINUE},
    {"default",     CUP_TOK_DEFAULT},
    {"do",          CUP_TOK_DO},
    {"double",      CUP_TOK_DOUBLE},
    {"else",        CUP_TOK_ELSE},
    {"enum",        CUP_TOK_ENUM},
    {"extern",      CUP_TOK_EXTERN},
    {"float",       CUP_TOK_FLOAT},
    {"for",         CUP_TOK_FOR},
    {"goto",        CUP_TOK_GOTO},
    {"if",          CUP_TOK_IF},
    {"inline",      CUP_TOK_INLINE},
    {"int",         CUP_TOK_INT},
    {"long",        CUP_TOK_LONG},
    {"register",    CUP_TOK_REGISTER},
    {"restrict",    CUP_TOK_RESTRICT},
    {"return",      CUP_TOK_RETURN},
    {"short",       CUP_TOK_SHORT},
    {"signed",      CUP_TOK_SIGNED},
    {"sizeof",      CUP_TOK_SIZEOF},
    {"static",      CUP_TOK_STATIC},
    {"struct",      CUP_TOK_STRUCT},
    {"switch",      CUP_TOK_SWITCH},
    {"typedef",     CUP_TOK_TYPEDEF},
    {"union",       CUP_TOK_UNION},
    {"unsigned",    CUP_TOK_UNSIGNED},
    {"void",        CUP_TOK_VOID},
    {"volatile",    CUP_TOK_VOLATILE},
    {"while",       CUP_TOK_WHILE},
    {"_Bool",       CUP_TOK__BOOL},
    {"_Complex",    CUP_TOK__COMPLEX},
    {"_Imaginary",  CUP_TOK__IMAGINARY},
    {"_Atomic",     CUP_TOK__ATOMIC},
    {"_Static_assert", CUP_TOK__STATIC_ASSERT},
    {"_Generic",    CUP_TOK__GENERIC},
    {"_Noreturn",   CUP_TOK__NORETURN},
    {"_Alignas",    CUP_TOK__ALIGNAS},
    {"_Alignof",    CUP_TOK__ALIGNOF},
    {NULL, 0}
};

static int cu_lookup_keyword(const char *s, int len)
{
    for (int i = 0; g_keywords[i].name; i++) {
        if ((int)strlen(g_keywords[i].name) == len &&
            strncmp(g_keywords[i].name, s, len) == 0) {
            return g_keywords[i].kind;
        }
    }
    return 0;
}

const char *cu_tok_keyword_name(int kind)
{
    if (kind < CUP_TOK_KW_BASE) return NULL;
    for (int i = 0; g_keywords[i].name; i++) {
        if (g_keywords[i].kind == kind) return g_keywords[i].name;
    }
    return NULL;
}

int cu_tok_is_type_kw(int kind)
{
    switch (kind) {
        case CUP_TOK_VOID:
        case CUP_TOK_CHAR_KW:
        case CUP_TOK_SHORT:
        case CUP_TOK_INT:
        case CUP_TOK_LONG:
        case CUP_TOK_FLOAT:
        case CUP_TOK_DOUBLE:
        case CUP_TOK_SIGNED:
        case CUP_TOK_UNSIGNED:
        case CUP_TOK__BOOL:
        case CUP_TOK__COMPLEX:
        case CUP_TOK__IMAGINARY:
        case CUP_TOK_STRUCT:
        case CUP_TOK_UNION:
        case CUP_TOK_ENUM:
            return 1;
        default:
            return 0;
    }
}

int cu_tok_is_storage_kw(int kind)
{
    switch (kind) {
        case CUP_TOK_TYPEDEF:
        case CUP_TOK_EXTERN:
        case CUP_TOK_STATIC:
        case CUP_TOK_AUTO:
        case CUP_TOK_REGISTER:
        case CUP_TOK_INLINE:
        case CUP_TOK__NORETURN:
            return 1;
        default:
            return 0;
    }
}

int cu_tok_is_qualifier_kw(int kind)
{
    switch (kind) {
        case CUP_TOK_CONST:
        case CUP_TOK_VOLATILE:
        case CUP_TOK_RESTRICT:
        case CUP_TOK__ATOMIC:
            return 1;
        default:
            return 0;
    }
}

/* ------------------------------------------------------------------ */
/* 内部辅助                                                            */
/* ------------------------------------------------------------------ */

static void cu_token_init(CuToken *t)
{
    t->kind = 0;
    t->str = NULL;
    t->ival = 0;
    t->fval = 0.0;
    t->is_float = 0;
    t->line = 0;
    t->col = 0;
}

static void cu_token_free(CuToken *t)
{
    if (t->str) {
        free(t->str);
        t->str = NULL;
    }
}

/* 复制 token 内容（深拷贝 str） */
static void cu_token_copy(CuToken *dst, const CuToken *src)
{
    cu_token_free(dst);
    dst->kind = src->kind;
    dst->str = src->str ? strdup(src->str) : NULL;
    dst->ival = src->ival;
    dst->fval = src->fval;
    dst->is_float = src->is_float;
    dst->line = src->line;
    dst->col = src->col;
}

static char cu_peek_char(CuLexer *lex, int offset)
{
    size_t p = lex->pos + offset;
    if (p >= lex->src_len) return '\0';
    return lex->src[p];
}

static char cu_read_char(CuLexer *lex)
{
    if (lex->pos >= lex->src_len) return '\0';
    char c = lex->src[lex->pos++];
    if (c == '\n') {
        lex->line++;
        lex->col = 1;
    } else {
        lex->col++;
    }
    return c;
}

void cu_lex_error(CuLexer *lex, const char *msg)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s:%d:%d: 词法错误: %s",
             lex->filename ? lex->filename : "<input>",
             lex->line, lex->col, msg);
    if (lex->result) {
        cupdate_result_add_error(lex->result, buf, lex->line);
    }
}

/* 跳过空白和注释（不跨行则不消耗换行） */
static void cu_skip_ws(CuLexer *lex)
{
    while (lex->pos < lex->src_len) {
        char c = cu_peek_char(lex, 0);
        fprintf(stderr, "DEBUG cu_skip_ws: checking pos=%zu c='%c'(%d)\n",
                lex->pos, c, (unsigned char)c);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v') {
            cu_read_char(lex);
            continue;
        }
        if (c == '\\') {
            /* 行连接：反斜杠 + 换行 */
            if (cu_peek_char(lex, 1) == '\n') {
                cu_read_char(lex);
                cu_read_char(lex);
                continue;
            }
            break;
        }
        if (c == '/' && cu_peek_char(lex, 1) == '/') {
            /* 行注释 */
            fprintf(stderr, "DEBUG cu_skip_ws: skipping line comment at pos=%zu\n", lex->pos);
            while (lex->pos < lex->src_len && cu_peek_char(lex, 0) != '\n')
                cu_read_char(lex);
            continue;
        }
        if (c == '/' && cu_peek_char(lex, 1) == '*') {
            /* 块注释 */
            fprintf(stderr, "DEBUG cu_skip_ws: skipping block comment at pos=%zu\n", lex->pos);
            cu_read_char(lex);
            cu_read_char(lex);
            while (lex->pos < lex->src_len) {
                if (cu_peek_char(lex, 0) == '*' && cu_peek_char(lex, 1) == '/') {
                    cu_read_char(lex);
                    cu_read_char(lex);
                    break;
                }
                cu_read_char(lex);
            }
            continue;
        }
        break;
    }
    fprintf(stderr, "DEBUG cu_skip_ws: done, pos=%zu c='%c'(%d)\n",
            lex->pos, lex->pos < lex->src_len ? cu_peek_char(lex, 0) : 'E',
            lex->pos < lex->src_len ? (unsigned char)cu_peek_char(lex, 0) : -1);
}

/* 解析字符串/字符转义序列，返回字符值，p 指向反斜杠后第一个字符 */
static int cu_parse_escape(CuLexer *lex, size_t *p)
{
    char c = lex->src[*p];
    (*p)++;
    switch (c) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            /* 八进制 */
            int val = c - '0';
            int cnt = 1;
            while (cnt < 3 && *p < lex->src_len && lex->src[*p] >= '0' && lex->src[*p] <= '7') {
                val = val * 8 + (lex->src[*p] - '0');
                (*p)++;
                cnt++;
            }
            return val;
        }
        case 'x': {
            /* 十六进制 */
            int val = 0;
            while (*p < lex->src_len && isxdigit((unsigned char)lex->src[*p])) {
                char h = lex->src[*p];
                int d = (h >= '0' && h <= '9') ? h - '0' :
                        (h >= 'a' && h <= 'f') ? h - 'a' + 10 : h - 'A' + 10;
                val = val * 16 + d;
                (*p)++;
            }
            return val;
        }
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'v': return '\v';
        case '?': return '?';
        case 'e': return 27;  /* GCC 扩展 */
        default:
            return (unsigned char)c;
    }
}

/* 读取字符串字面量 */
static void cu_read_string(CuLexer *lex, char quote, CuToken *out)
{
    /* quote = '"' (字符串) 或 '\'' (字符) */
    int start_line = lex->line;
    int start_col = lex->col;

    /* 收集到 buffer */
    size_t cap = 32;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { out->kind = CUP_TOK_ERROR; return; }

    cu_read_char(lex);  /* 消费开头的引号 */

    while (lex->pos < lex->src_len) {
        char c = cu_peek_char(lex, 0);
        if (c == quote) {
            cu_read_char(lex);
            break;
        }
        if (c == '\n') {
            cu_lex_error(lex, "未结束的字符串/字符字面量");
            break;
        }
        if (c == '\\') {
            cu_read_char(lex);  /* 消费反斜杠 */
            size_t p = lex->pos;
            int ch = cu_parse_escape(lex, &p);
            /* 推进 lex 位置到 p，并粗略更新列号 */
            int consumed = (int)(p - lex->pos);
            lex->pos = p;
            lex->col += consumed;
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = (char)ch;
            continue;
        }
        cu_read_char(lex);
        if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
        buf[len++] = c;
    }
    buf[len] = '\0';

    out->kind = (quote == '"') ? CUP_TOK_STR : CUP_TOK_CHAR;
    out->str = buf;
    out->line = start_line;
    out->col = start_col;
    if (quote == '\'') {
        /* 字符字面量的值 */
        out->ival = (len > 0) ? (unsigned char)buf[0] : 0;
    }
}

/* 读取数字字面量 */
static void cu_read_number(CuLexer *lex, CuToken *out)
{
    int start_line = lex->line;
    int start_col = lex->col;

    size_t cap = 32;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { out->kind = CUP_TOK_ERROR; return; }

    int is_float = 0;
    int is_hex = 0;

    char c = cu_peek_char(lex, 0);
    if (c == '0' && (cu_peek_char(lex, 1) == 'x' || cu_peek_char(lex, 1) == 'X')) {
        is_hex = 1;
        buf[len++] = cu_read_char(lex);
        buf[len++] = cu_read_char(lex);
        while (lex->pos < lex->src_len) {
            char d = cu_peek_char(lex, 0);
            if (isxdigit((unsigned char)d) || d == '_') {
                if (d != '_') {
                    if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                    buf[len++] = cu_read_char(lex);
                } else {
                    cu_read_char(lex);
                }
            } else break;
        }
    } else {
        /* 十进制整数或浮点 */
        while (lex->pos < lex->src_len) {
            char d = cu_peek_char(lex, 0);
            if (isdigit((unsigned char)d)) {
                if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                buf[len++] = cu_read_char(lex);
            } else if (d == '_') {
                cu_read_char(lex);
            } else break;
        }
        /* 浮点：. e/E */
        if (cu_peek_char(lex, 0) == '.' && isdigit((unsigned char)cu_peek_char(lex, 1))) {
            is_float = 1;
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = cu_read_char(lex);
            while (lex->pos < lex->src_len && isdigit((unsigned char)cu_peek_char(lex, 0))) {
                if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                buf[len++] = cu_read_char(lex);
            }
        }
        if (cu_peek_char(lex, 0) == 'e' || cu_peek_char(lex, 0) == 'E') {
            is_float = 1;
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = cu_read_char(lex);
            if (cu_peek_char(lex, 0) == '+' || cu_peek_char(lex, 0) == '-') {
                if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                buf[len++] = cu_read_char(lex);
            }
            while (lex->pos < lex->src_len && isdigit((unsigned char)cu_peek_char(lex, 0))) {
                if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
                buf[len++] = cu_read_char(lex);
            }
        }
    }

    /* 后缀：u U l L f F */
    while (lex->pos < lex->src_len) {
        char s = cu_peek_char(lex, 0);
        if (s == 'u' || s == 'U' || s == 'l' || s == 'L' || s == 'f' || s == 'F') {
            if (s == 'f' || s == 'F') is_float = 1;
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = cu_read_char(lex);
        } else break;
    }

    buf[len] = '\0';

    out->kind = CUP_TOK_NUM;
    out->str = buf;
    out->is_float = is_float;
    if (is_float) {
        out->fval = strtod(buf, NULL);
    } else {
        if (is_hex) out->ival = strtol(buf, NULL, 16);
        else out->ival = strtol(buf, NULL, 10);
    }
    out->line = start_line;
    out->col = start_col;
}

/* 读取标识符 */
static void cu_read_identifier(CuLexer *lex, CuToken *out)
{
    int start_line = lex->line;
    int start_col = lex->col;

    size_t cap = 32;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { out->kind = CUP_TOK_ERROR; return; }

    while (lex->pos < lex->src_len) {
        char c = cu_peek_char(lex, 0);
        if (isalpha((unsigned char)c) || c == '_' || isdigit((unsigned char)c) ||
            (c & 0x80)) {  /* UTF-8 字节 */
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = cu_read_char(lex);
        } else break;
    }
    buf[len] = '\0';

    fprintf(stderr, "DEBUG cu_read_identifier: buf='%s' len=%zu\n", buf, len);

    int kw = cu_lookup_keyword(buf, (int)len);
    if (kw) {
        out->kind = kw;
        out->str = buf;  /* 关键字也保留字符串 */
    } else {
        out->kind = CUP_TOK_ID;
        out->str = buf;
    }
    out->line = start_line;
    out->col = start_col;
}

/* 读取预处理器指令内容（# 到行尾） */
static void cu_read_pp_line(CuLexer *lex, CuToken *out)
{
    int start_line = lex->line;
    int start_col = lex->col;

    cu_read_char(lex);  /* 消费 # */
    /* 跳过 # 后的空白（不跨行） */
    while (lex->pos < lex->src_len) {
        char c = cu_peek_char(lex, 0);
        if (c == ' ' || c == '\t') cu_read_char(lex);
        else break;
    }

    /* 读取指令名（标识符） */
    size_t cap = 32;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { out->kind = CUP_TOK_ERROR; return; }

    while (lex->pos < lex->src_len) {
        char c = cu_peek_char(lex, 0);
        if (isalpha((unsigned char)c) || c == '_' || isdigit((unsigned char)c)) {
            if (len + 1 >= cap) { cap *= 2; buf = (char *)realloc(buf, cap); }
            buf[len++] = cu_read_char(lex);
        } else break;
    }
    buf[len] = '\0';

    out->kind = CUP_TOK_PP;
    out->str = buf;
    out->line = start_line;
    out->col = start_col;
}

/* ------------------------------------------------------------------ */
/* 对外接口                                                            */
/* ------------------------------------------------------------------ */

/* 前向声明：内部扫描函数 */
static int cu_lex_scan(CuLexer *lex);

void cu_lex_init(CuLexer *lex, const char *source, const char *filename,
                 struct CUPResult *result)
{
    lex->src = source;
    lex->src_len = strlen(source);
    lex->pos = 0;
    lex->line = 1;
    lex->col = 1;
    lex->filename = filename;
    lex->at_line_start = 1;
    lex->has_peek = 0;
    cu_token_init(&lex->cur);
    cu_token_init(&lex->peek);
    lex->result = result;

    fprintf(stderr, "DEBUG cu_lex_init: src_len=%zu\n", lex->src_len);
    fprintf(stderr, "DEBUG cu_lex_init: src=%.160s\n", source);

    /* 预扫描第一个 token 到 cur */
    cu_lex_scan(lex);
}

void cu_lex_free(CuLexer *lex)
{
    cu_token_free(&lex->cur);
    cu_token_free(&lex->peek);
}

/* 内部：从源码扫描下一个 token，无 peek 缓存 */
static int cu_lex_scan(CuLexer *lex)
{
    while (1) {
        cu_skip_ws(lex);
        if (lex->pos >= lex->src_len) {
            lex->cur.kind = CUP_TOK_EOF;
            lex->cur.str = NULL;
            lex->cur.line = lex->line;
            lex->cur.col = lex->col;
            return CUP_TOK_EOF;
        }

        int start_line = lex->line;
        int start_col = lex->col;
        char c = cu_peek_char(lex, 0);
        char c2 = cu_peek_char(lex, 1);
        char c3 = cu_peek_char(lex, 2);

        fprintf(stderr, "DEBUG lex_scan: pos=%zu c='%c'(%d) at_line_start=%d\n",
                lex->pos, c, (unsigned char)c, lex->at_line_start);

        /* 行首的 #：预处理器指令 */
        if (lex->at_line_start && c == '#') {
            cu_read_pp_line(lex, &lex->cur);
            lex->at_line_start = 0;
            fprintf(stderr, "DEBUG lex: PP token '%s' at line %d\n",
                    lex->cur.str ? lex->cur.str : "<null>", lex->cur.line);
            return lex->cur.kind;
        }

        /* 换行 */
        if (c == '\n') {
            cu_read_char(lex);
            lex->at_line_start = 1;
            continue;
        }

        lex->at_line_start = 0;

        /* 标识符 / 关键字 */
        if (isalpha((unsigned char)c) || c == '_' || (c & 0x80)) {
            cu_read_identifier(lex, &lex->cur);
            fprintf(stderr, "DEBUG lex: ID token '%s' kind=%d at line %d\n",
                    lex->cur.str ? lex->cur.str : "<null>", lex->cur.kind, lex->cur.line);
            return lex->cur.kind;
        }

        /* 数字 */
        if (isdigit((unsigned char)c)) {
            cu_read_number(lex, &lex->cur);
            return lex->cur.kind;
        }
        /* . 后跟数字（浮点） */
        if (c == '.' && isdigit((unsigned char)c2)) {
            cu_read_number(lex, &lex->cur);
            return lex->cur.kind;
        }

        /* 字符串/字符字面量 */
        if (c == '"' || c == '\'') {
            /* 字符串前缀：L u U u8 */
            /* 此处简化：直接读取，前缀已被标识符扫描吸收，但若以 L" 形式出现，
             * 上一个 token 已为 L 标识符。此处仅处理普通字符串。 */
            cu_read_string(lex, c, &lex->cur);
            return lex->cur.kind;
        }

        /* 多字符运算符 */
        if (c == '.' && c2 == '.' && c3 == '.') {
            cu_read_char(lex); cu_read_char(lex); cu_read_char(lex);
            lex->cur.kind = CUP_TOK_ELLIPSIS;
            lex->cur.str = NULL;
            lex->cur.line = start_line;
            lex->cur.col = start_col;
            return CUP_TOK_ELLIPSIS;
        }
        if (c == '<' && c2 == '<' && c3 == '=') {
            cu_read_char(lex); cu_read_char(lex); cu_read_char(lex);
            lex->cur.kind = CUP_TOK_SHL_EQ;
            lex->cur.str = NULL;
            lex->cur.line = start_line;
            lex->cur.col = start_col;
            return CUP_TOK_SHL_EQ;
        }
        if (c == '>' && c2 == '>' && c3 == '=') {
            cu_read_char(lex); cu_read_char(lex); cu_read_char(lex);
            lex->cur.kind = CUP_TOK_SHR_EQ;
            lex->cur.str = NULL;
            lex->cur.line = start_line;
            lex->cur.col = start_col;
            return CUP_TOK_SHR_EQ;
        }
        if (c == '#' && c2 == '#') {
            cu_read_char(lex); cu_read_char(lex);
            lex->cur.kind = CUP_TOK_HASH_HASH;
            lex->cur.str = NULL;
            lex->cur.line = start_line;
            lex->cur.col = start_col;
            return CUP_TOK_HASH_HASH;
        }

        switch (c) {
            case '+':
                if (c2 == '+') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_INC; goto ret2; }
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_PLUS_EQ; goto ret2; }
                break;
            case '-':
                if (c2 == '-') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_DEC; goto ret2; }
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_MINUS_EQ; goto ret2; }
                if (c2 == '>') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_PTR; goto ret2; }
                break;
            case '*':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_MUL_EQ; goto ret2; }
                break;
            case '/':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_DIV_EQ; goto ret2; }
                break;
            case '%':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_MOD_EQ; goto ret2; }
                break;
            case '&':
                if (c2 == '&') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_AND_AND; goto ret2; }
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_AND_EQ; goto ret2; }
                break;
            case '|':
                if (c2 == '|') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_OR_OR; goto ret2; }
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_OR_EQ; goto ret2; }
                break;
            case '^':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_XOR_EQ; goto ret2; }
                break;
            case '=':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_EQ; goto ret2; }
                break;
            case '!':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_NEQ; goto ret2; }
                break;
            case '<':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_LE; goto ret2; }
                if (c2 == '<') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_SHL; goto ret2; }
                break;
            case '>':
                if (c2 == '=') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_GE; goto ret2; }
                if (c2 == '>') { cu_read_char(lex); cu_read_char(lex); lex->cur.kind = CUP_TOK_SHR; goto ret2; }
                break;
        }

        /* 单字符 token */
        cu_read_char(lex);
        lex->cur.kind = (unsigned char)c;
        lex->cur.str = NULL;
        lex->cur.line = start_line;
        lex->cur.col = start_col;
        return lex->cur.kind;

    ret2:
        lex->cur.str = NULL;
        lex->cur.line = start_line;
        lex->cur.col = start_col;
        return lex->cur.kind;
    }
}

/* ------------------------------------------------------------------ */
/* 公共接口：next / cur / peek                                          */
/* ------------------------------------------------------------------ */

int cu_lex_cur(CuLexer *lex)
{
    return lex->cur.kind;
}

int cu_lex_peek(CuLexer *lex)
{
    if (!lex->has_peek) {
        /* 扫描下一个 token 到 peek */
        /* 保存当前 cur，扫描下一个，存入 peek，再恢复 cur。
         * 注意 saved 必须先初始化（cu_token_copy 会先 cu_token_free(dst)，
         * 未初始化的 saved.str 可能是任意值，导致 free 崩溃）。 */
        CuToken saved;
        cu_token_init(&saved);
        cu_token_copy(&saved, &lex->cur);
        cu_token_free(&lex->cur);
        cu_lex_scan(lex);
        cu_token_copy(&lex->peek, &lex->cur);
        cu_token_free(&lex->cur);
        cu_token_copy(&lex->cur, &saved);
        cu_token_free(&saved);
        lex->has_peek = 1;
    }
    return lex->peek.kind;
}

int cu_lex_next(CuLexer *lex)
{
    if (lex->has_peek) {
        /* peek 变为新 cur */
        cu_token_free(&lex->cur);
        cu_token_copy(&lex->cur, &lex->peek);
        cu_token_free(&lex->peek);
        lex->has_peek = 0;
    } else {
        /* 直接扫描下一个 */
        cu_token_free(&lex->cur);
        cu_lex_scan(lex);
    }
    return lex->cur.kind;
}
