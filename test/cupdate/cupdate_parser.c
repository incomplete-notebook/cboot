/*
 * CBoot - C Project Bootstrapping Tool v0.3.1
 * C 语法分析器实现
 *
 * 参考 tinycc tccgen.c 的 decl() / parse_btype() / type_decl() / struct_decl()。
 * 简化为：仅识别顶层声明，不做语义分析，不生成代码。
 *
 * 主要任务：
 *   - 识别 typedef / struct / union / enum / 函数 / 全局变量 / #define / #include
 *   - 提取名称、类型、参数/成员
 *   - 收集语法错误
 */

#include "cupdate_parser.h"
#include "cupdate_lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------------------------------------------------------ */
/* 辅助：动态字符串                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} SB;  /* string builder */

static void sb_init(SB *s)
{
    s->cap = 32;
    s->len = 0;
    s->buf = (char *)malloc(s->cap);
    s->buf[0] = '\0';
}

static void sb_free(SB *s)
{
    if (s->buf) { free(s->buf); s->buf = NULL; }
    s->len = s->cap = 0;
}

static void sb_reserve(SB *s, size_t extra)
{
    if (s->len + extra + 1 > s->cap) {
        while (s->len + extra + 1 > s->cap) s->cap *= 2;
        s->buf = (char *)realloc(s->buf, s->cap);
    }
}

static void sb_append(SB *s, const char *str)
{
    size_t n = strlen(str);
    sb_reserve(s, n);
    memcpy(s->buf + s->len, str, n);
    s->len += n;
    s->buf[s->len] = '\0';
}

static void sb_append_char(SB *s, char c)
{
    sb_reserve(s, 1);
    s->buf[s->len++] = c;
    s->buf[s->len] = '\0';
}

static void sb_append_token(SB *s, CuLexer *lex)
{
    /* 根据当前 token 类型附加其文本 */
    int k = lex->cur.kind;
    if (k == CUP_TOK_ID || k == CUP_TOK_NUM || k == CUP_TOK_STR ||
        k == CUP_TOK_CHAR || (k >= CUP_TOK_KW_BASE && k < CUP_TOK_EOF)) {
        if (lex->cur.str) sb_append(s, lex->cur.str);
        else {
            char tmp[2] = {(char)k, 0};
            sb_append(s, tmp);
        }
    } else if (k >= 256 && k < CUP_TOK_KW_BASE) {
        /* 多字符运算符 */
        const char *op = NULL;
        switch (k) {
            case CUP_TOK_OR_OR:     op = "||"; break;
            case CUP_TOK_AND_AND:   op = "&&"; break;
            case CUP_TOK_EQ:        op = "=="; break;
            case CUP_TOK_NEQ:       op = "!="; break;
            case CUP_TOK_LE:        op = "<="; break;
            case CUP_TOK_GE:        op = ">="; break;
            case CUP_TOK_SHL:       op = "<<"; break;
            case CUP_TOK_SHR:       op = ">>"; break;
            case CUP_TOK_PLUS_EQ:   op = "+="; break;
            case CUP_TOK_MINUS_EQ:  op = "-="; break;
            case CUP_TOK_MUL_EQ:    op = "*="; break;
            case CUP_TOK_DIV_EQ:    op = "/="; break;
            case CUP_TOK_MOD_EQ:    op = "%="; break;
            case CUP_TOK_AND_EQ:    op = "&="; break;
            case CUP_TOK_OR_EQ:     op = "|="; break;
            case CUP_TOK_XOR_EQ:    op = "^="; break;
            case CUP_TOK_SHL_EQ:    op = "<<="; break;
            case CUP_TOK_SHR_EQ:    op = ">>="; break;
            case CUP_TOK_INC:       op = "++"; break;
            case CUP_TOK_DEC:       op = "--"; break;
            case CUP_TOK_PTR:       op = "->"; break;
            case CUP_TOK_ELLIPSIS:  op = "..."; break;
            case CUP_TOK_HASH_HASH: op = "##"; break;
            default:                op = "?"; break;
        }
        sb_append(s, op);
    } else if (k < 256) {
        char tmp[2] = {(char)k, 0};
        sb_append(s, tmp);
    }
}

/* trim trailing whitespace from string builder */
static void sb_trim(SB *s)
{
    while (s->len > 0 && (s->buf[s->len - 1] == ' ' || s->buf[s->len - 1] == '\t')) {
        s->buf[--s->len] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* 辅助：错误报告                                                      */
/* ------------------------------------------------------------------ */

static void cup_error(CuParser *p, const char *msg)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s:%d: 错误: %s",
             p->filename ? p->filename : "<input>",
             p->lex->cur.line, msg);
    cupdate_result_add_error(p->result, buf, p->lex->cur.line);
}

static void cup_error_at(CuParser *p, int line, const char *msg)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s:%d: 错误: %s",
             p->filename ? p->filename : "<input>", line, msg);
    cupdate_result_add_error(p->result, buf, line);
}

static void cup_warning(CuParser *p, const char *msg)
{
    char buf[512];
    snprintf(buf, sizeof(buf), "%s:%d: 警告: %s",
             p->filename ? p->filename : "<input>",
             p->lex->cur.line, msg);
    cupdate_result_add_warning(p->result, buf, p->lex->cur.line);
}

/* ------------------------------------------------------------------ */
/* Token 操作辅助                                                      */
/* ------------------------------------------------------------------ */

#define CUR(p)     ((p)->lex->cur.kind)
#define PEEK(p)    cu_lex_peek((p)->lex)
#define NEXT(p)    cu_lex_next((p)->lex)

static int accept(CuParser *p, int kind)
{
    if (CUR(p) == kind) {
        NEXT(p);
        return 1;
    }
    return 0;
}

static void expect(CuParser *p, int kind, const char *what)
{
    if (!accept(p, kind)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "期望 '%s'", what);
        cup_error(p, buf);
    }
}

/* 跳过直到遇到指定的 token（不消费） */
static void cup_skip_to(CuParser *p, int kind)
{
    while (CUR(p) != kind && CUR(p) != CUP_TOK_EOF) {
        NEXT(p);
    }
}

/* 跳过直到遇到指定的两个 token 之一（不消费） */
static void cup_skip_to2(CuParser *p, int k1, int k2)
{
    while (CUR(p) != k1 && CUR(p) != k2 && CUR(p) != CUP_TOK_EOF) {
        NEXT(p);
    }
}

/* 跳过平衡的括号/大括号/方括号，从当前 ( [ { 开始，到 ) ] } 结束。
 * 当前 token 必须是开括号。返回 0 成功，-1 失败。 */
static int cup_skip_balanced(CuParser *p)
{
    int open = CUR(p);
    int close;
    int depth = 1;
    if (open == '(') close = ')';
    else if (open == '{') close = '}';
    else if (open == '[') close = ']';
    else return -1;
    NEXT(p);
    while (CUR(p) != CUP_TOK_EOF) {
        int k = CUR(p);
        if (k == open) {
            depth++;
        } else if (k == close) {
            depth--;
            if (depth == 0) {
                NEXT(p);
                return 0;
            }
        }
        NEXT(p);
    }
    cup_error_at(p, p->lex->cur.line, "未闭合的括号");
    return -1;
}

/* ------------------------------------------------------------------ */
/* 类型解析                                                            */
/* ------------------------------------------------------------------ */

/* 判断当前 token 是否可以开始一个类型说明符 */
static int cup_is_type_start(CuParser *p)
{
    int k = CUR(p);
    if (cu_tok_is_type_kw(k)) return 1;
    if (k == CUP_TOK_CONST || k == CUP_TOK_VOLATILE || k == CUP_TOK_RESTRICT ||
        k == CUP_TOK__ATOMIC) return 1;
    /* typedef 名作为类型：CUP_TOK_ID 当且仅当下一个 token 是 ID/星号/... */
    /* 简化：若 ID 后面跟 ID 或 * 或 ( 视为类型 */
    if (k == CUP_TOK_ID) {
        int pk = PEEK(p);
        if (pk == CUP_TOK_ID || pk == '*' || pk == '&' || pk == CUP_TOK_CONST ||
            pk == CUP_TOK_VOLATILE || pk == CUP_TOK_RESTRICT) {
            return 1;
        }
    }
    return 0;
}

/* 解析类型说明符（base type），收集到 type_sb。
 * 处理：storage class, qualifier, 基础类型, struct/union/enum, typedef 名。
 * 不消费 declarator 部分（*, (), []）。
 * 返回类型种类：'b' 基础, 's' struct, 'u' union, 'e' enum, 't' typedef 名, 0 失败。
 * 对于 struct/union/enum，将名称填入 tag_out（动态分配）。
 * 注意：storage class（typedef/static/extern/inline 等）不写入 type_sb，
 *       只通过 is_*_out 输出参数返回。 */
static char cup_parse_type_specifier(CuParser *p, SB *type_sb,
                                      char **tag_out, int *is_static_out,
                                      int *is_inline_out, int *is_extern_out,
                                      int *is_typedef_out)
{
    if (tag_out) *tag_out = NULL;
    if (is_static_out) *is_static_out = 0;
    if (is_inline_out) *is_inline_out = 0;
    if (is_extern_out) *is_extern_out = 0;
    if (is_typedef_out) *is_typedef_out = 0;

    int saw_type = 0;
    char kind = 'b';

    while (1) {
        int k = CUR(p);
        if (k == CUP_TOK_STATIC) {
            if (is_static_out) *is_static_out = 1;
            NEXT(p);
            continue;
        }
        if (k == CUP_TOK_INLINE) {
            if (is_inline_out) *is_inline_out = 1;
            NEXT(p);
            continue;
        }
        if (k == CUP_TOK_EXTERN) {
            if (is_extern_out) *is_extern_out = 1;
            NEXT(p);
            continue;
        }
        if (k == CUP_TOK_TYPEDEF) {
            if (is_typedef_out) *is_typedef_out = 1;
            NEXT(p);
            continue;
        }
        if (k == CUP_TOK_AUTO || k == CUP_TOK_REGISTER || k == CUP_TOK__NORETURN) {
            NEXT(p);
            continue;
        }
        if (k == CUP_TOK_CONST || k == CUP_TOK_VOLATILE || k == CUP_TOK_RESTRICT ||
            k == CUP_TOK__ATOMIC) {
            sb_append_token(type_sb, p->lex); sb_append_char(type_sb, ' ');
            NEXT(p);
            continue;
        }
        if (cu_tok_is_type_kw(k) && k != CUP_TOK_STRUCT && k != CUP_TOK_UNION && k != CUP_TOK_ENUM) {
            sb_append_token(type_sb, p->lex); sb_append_char(type_sb, ' ');
            NEXT(p);
            saw_type = 1;
            continue;
        }
        if (k == CUP_TOK_STRUCT || k == CUP_TOK_UNION || k == CUP_TOK_ENUM) {
            if (saw_type) break;  /* 已经有类型，不能再叠加 struct/union/enum */
            kind = (k == CUP_TOK_STRUCT) ? 's' : (k == CUP_TOK_UNION) ? 'u' : 'e';
            sb_append_token(type_sb, p->lex); sb_append_char(type_sb, ' ');
            NEXT(p);
            /* 可选 tag */
            if (CUR(p) == CUP_TOK_ID) {
                if (tag_out) *tag_out = strdup(p->lex->cur.str);
                sb_append_token(type_sb, p->lex); sb_append_char(type_sb, ' ');
                NEXT(p);
            }
            /* 可选 { 成员 }：留给 cup_parse_struct_decl 等专门处理，不在此消费 */
            if (CUR(p) == '{') {
                saw_type = 1;
                break;
            }
            saw_type = 1;
            continue;
        }
        /* typedef 名 */
        if (k == CUP_TOK_ID && !saw_type) {
            /* 检查下一个 token：若是 ID 或 * 或 ( 等，则本 ID 是类型 */
            int pk = PEEK(p);
            if (pk == CUP_TOK_ID || pk == '*' || pk == '&' || pk == '(' ||
                pk == CUP_TOK_CONST || pk == CUP_TOK_VOLATILE || pk == '[') {
                kind = 't';
                sb_append_token(type_sb, p->lex); sb_append_char(type_sb, ' ');
                NEXT(p);
                saw_type = 1;
                continue;
            }
        }
        break;
    }

    if (!saw_type) {
        return 0;
    }
    sb_trim(type_sb);
    return kind;
}

/* ------------------------------------------------------------------ */
/* 声明子解析：解析指针、数组、函数 declarator                          */
/* ------------------------------------------------------------------ */

/* 前向声明 */
static char *cup_build_full_type(SB *type_sb, const char *suffix);

/* 从 type_sb（已有 base type）开始，解析 declarator，提取名称。
 * 返回名称（动态分配，调用方负责释放），NULL 表示匿名/失败。
 * suffix_out: 返回后缀字符串（如 "(int,int)" 或 "[32]"），调用方负责释放。 */
static char *cup_parse_declarator(CuParser *p, SB *type_sb,
                                   CUParParam **params_out, int *param_count_out,
                                   int *is_function_out,
                                   int *is_function_def_out,
                                   char **suffix_out)
{
    if (params_out) *params_out = NULL;
    if (param_count_out) *param_count_out = 0;
    if (is_function_out) *is_function_out = 0;
    if (is_function_def_out) *is_function_def_out = 0;
    if (suffix_out) *suffix_out = NULL;

    /* 收集前缀星号/限定符：int * const * name
     * 星号直接追加到 type_sb（空格已由 type_specifier 保证）。 */
    int star_count = 0;
    while (CUR(p) == '*' || CUR(p) == '&' || CUR(p) == CUP_TOK_CONST ||
           CUR(p) == CUP_TOK_VOLATILE || CUR(p) == CUP_TOK_RESTRICT ||
           CUR(p) == CUP_TOK__ATOMIC) {
        if (CUR(p) == '*') {
            star_count++;
            /* 去除尾部空格后再加 *，确保 "struct Point*" 而非 "struct Point *" */
            while (type_sb->len > 0 && type_sb->buf[type_sb->len - 1] == ' ') {
                type_sb->buf[--type_sb->len] = '\0';
            }
            sb_append_char(type_sb, '*');
        }
        NEXT(p);
        /* 跳过 qualifier（不写入类型字符串） */
        while (CUR(p) == CUP_TOK_CONST || CUR(p) == CUP_TOK_VOLATILE ||
               CUR(p) == CUP_TOK_RESTRICT || CUR(p) == CUP_TOK__ATOMIC) {
            NEXT(p);
        }
    }

    /* 检测 (name) 或 (*name) 模式：
     * int (*FuncPtr)(int, int)  → 函数指针 declarator
     * int (* const FuncPtr)(int) → 带限定符的函数指针
     * int (FuncPtr)(int)       → 仅括号包裹的普通函数
     * int (*p)                  → 括号包裹的指针变量 */
    int parenthesized = 0;
    int inner_star_count = 0;
    if (CUR(p) == '(') {
        NEXT(p);
        parenthesized = 1;
        while (CUR(p) == '*' || CUR(p) == CUP_TOK_CONST ||
               CUR(p) == CUP_TOK_VOLATILE || CUR(p) == CUP_TOK_RESTRICT ||
               CUR(p) == CUP_TOK__ATOMIC) {
            if (CUR(p) == '*') inner_star_count++;
            NEXT(p);
            while (CUR(p) == CUP_TOK_CONST || CUR(p) == CUP_TOK_VOLATILE ||
                   CUR(p) == CUP_TOK_RESTRICT || CUR(p) == CUP_TOK__ATOMIC) {
                NEXT(p);
            }
        }
    }

    char *name = NULL;
    if (CUR(p) == CUP_TOK_ID) {
        name = strdup(p->lex->cur.str);
        NEXT(p);
    }

    /* 消费闭括号 )，若是 parenthesized */
    if (parenthesized) {
        while (CUR(p) != ')' && CUR(p) != CUP_TOK_EOF) NEXT(p);
        if (CUR(p) == ')') NEXT(p);
    }

    /* 收集后缀：() 函数参数 或 [] 数组维度。
     * 后缀存入 suffix_out，不追加到 type_sb。
     * 这样调用方可按需组合：return_type 只用 type_sb，
     * 而 base_type = type_sb + suffix。 */
    SB suffix; sb_init(&suffix);
    CUParParam *func_params = NULL;
    int func_pcount = 0;

    while (CUR(p) == '(' || CUR(p) == '[') {
        if (CUR(p) == '(') {
            if (is_function_out) *is_function_out = 1;
            NEXT(p);

            /* 收集参数类型字符串用于构建 suffix */
            SB param_str; sb_init(&param_str);
            func_params = NULL;
            int pcount = 0, pcap = 0;

            while (CUR(p) != ')' && CUR(p) != CUP_TOK_EOF) {
                if (CUR(p) == CUP_TOK_VOID && PEEK(p) == ')') {
                    sb_append(&param_str, "void");
                    NEXT(p);
                    break;
                }
                if (CUR(p) == CUP_TOK_ELLIPSIS) {
                    sb_append(&param_str, "...");
                    NEXT(p);
                    if (pcount + 1 > pcap) {
                        pcap = pcap ? pcap * 2 : 4;
                        func_params = (CUParParam *)realloc(func_params, sizeof(CUParParam) * pcap);
                    }
                    func_params[pcount].type = strdup("...");
                    func_params[pcount].name = NULL;
                    pcount++;
                    break;
                }
                SB ptype; sb_init(&ptype);
                char *ptag = NULL;
                cup_parse_type_specifier(p, &ptype, &ptag, NULL, NULL, NULL, NULL);
                if (ptag) free(ptag);
                char *psuffix = NULL;
                char *pname = cup_parse_declarator(p, &ptype, NULL, NULL, NULL, NULL, &psuffix);
                sb_trim(&ptype);

                /* 用 cup_build_full_type 组合参数完整类型 */
                char *pfull = cup_build_full_type(&ptype, psuffix);
                free(psuffix);

                if (pfull && strlen(pfull) > 0) {
                    if (param_str.len > 0) sb_append_char(&param_str, ',');
                    sb_append(&param_str, pfull);
                }

                if (pcount + 1 > pcap) {
                    pcap = pcap ? pcap * 2 : 4;
                    func_params = (CUParParam *)realloc(func_params, sizeof(CUParParam) * pcap);
                }
                func_params[pcount].type = pfull;
                func_params[pcount].name = pname;
                pcount++;
                sb_free(&ptype);

                if (CUR(p) == ',') NEXT(p);
                else break;
            }
            expect(p, ')', ")");
            func_pcount = pcount;

            /* 构建函数 suffix：(params) 或 (*)(params) 用于函数指针 */
            sb_append_char(&suffix, '(');
            if (parenthesized && inner_star_count > 0) {
                sb_append(&suffix, "*");
                sb_append_char(&suffix, ')');
                sb_append_char(&suffix, '(');
                sb_append(&suffix, param_str.buf ? param_str.buf : "");
                sb_append_char(&suffix, ')');
            } else {
                sb_append(&suffix, param_str.buf ? param_str.buf : "");
                sb_append_char(&suffix, ')');
            }
            sb_free(&param_str);

            if (CUR(p) == '{' && is_function_def_out) {
                *is_function_def_out = 1;
            }
            break;
        } else {
            /* 数组：捕获维度文本 */
            SB dim; sb_init(&dim);
            NEXT(p);
            while (CUR(p) != ']' && CUR(p) != CUP_TOK_EOF) {
                sb_append_token(&dim, p->lex);
                NEXT(p);
            }
            expect(p, ']', "]");
            sb_append_char(&suffix, '[');
            sb_append(&suffix, dim.buf ? dim.buf : "");
            sb_append_char(&suffix, ']');
            sb_free(&dim);
        }
    }

    /* 将 suffix 返回给调用方，不写入 type_sb */
    if (suffix_out) {
        *suffix_out = strdup(suffix.buf ? suffix.buf : "");
    }
    sb_free(&suffix);

    if (params_out) *params_out = func_params;
    if (param_count_out) *param_count_out = func_pcount;

    return name;
}

/* 辅助：将 type_sb + suffix 组合成完整类型字符串 */
static char *cup_build_full_type(SB *type_sb, const char *suffix)
{
    if (!suffix || !*suffix) {
        return strdup(type_sb->buf ? type_sb->buf : "");
    }
    SB result; sb_init(&result);
    sb_append(&result, type_sb->buf ? type_sb->buf : "");
    /* 仅在后缀不是以 [ ( * 开头时才加空格（数组/函数/指针不需要空格） */
    char first = suffix[0];
    if (result.len > 0 && result.buf[result.len - 1] != '*' &&
        result.buf[result.len - 1] != ' ' &&
        first != '[' && first != '(' && first != '*') {
        sb_append_char(&result, ' ');
    }
    sb_append(&result, suffix);
    char *r = strdup(result.buf ? result.buf : "");
    sb_free(&result);
    return r;
}

/* ------------------------------------------------------------------ */
/* 函数体 / 初始化列表 跳过                                            */
/* ------------------------------------------------------------------ */

/* 收集从当前 { 到匹配 } 之间的原始源码文本（保留缩进和格式）。
 * 当前 token 必须是 {。结束后 cur 指向 } 之后。 */
static char *cup_capture_braced_body(CuParser *p)
{
    if (CUR(p) != '{') return NULL;
    int start_line = p->lex->cur.line;

    /* 记录 { 在源码中的起始位置 */
    size_t body_start = p->lex->cur.start_pos;

    NEXT(p);  /* 消费 { */

    int depth = 1;
    size_t body_end = body_start + 1;  /* 至少包含 { 字符 */

    while (CUR(p) != CUP_TOK_EOF) {
        int k = CUR(p);
        if (k == '{') {
            depth++;
            NEXT(p);
            continue;
        }
        if (k == '}') {
            depth--;
            if (depth == 0) {
                body_end = p->lex->cur.start_pos + 1;  /* } 之后的位置 */
                NEXT(p);  /* 消费 } */
                break;
            }
            NEXT(p);
            continue;
        }
        NEXT(p);
    }
    if (depth != 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "未闭合的函数体（起始于第 %d 行）", start_line);
        cup_error_at(p, start_line, buf);
    }

    /* 提取 { 到 } 之间的原始源码文本（不含外层大括号） */
    size_t inner_start = body_start + 1;  /* 跳过 { */
    size_t inner_len = 0;
    if (body_end > inner_start) {
        inner_len = body_end - 1 - inner_start;  /* 跳过 } */
    }

    char *result = (char *)malloc(inner_len + 1);
    if (!result) return strdup("");
    memcpy(result, p->lex->src + inner_start, inner_len);
    result[inner_len] = '\0';

    /* 去除首尾空白 */
    char *start = result;
    while (*start && isspace((unsigned char)*start)) start++;
    char *end = result + inner_len;
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
    if (start != result) {
        memmove(result, start, end - start + 1);
    }

    return result;
}

/* 跳过初始化列表或表达式，到 ; 或 , 之前。
 * 用于全局变量声明的初始化。返回捕获的初始化表达式原始文本。 */
static char *cup_capture_initializer(CuParser *p)
{
    /* 记录起始位置 */
    size_t init_start = p->lex->cur.start_pos;

    SB init; sb_init(&init);
    int depth = 0;
    size_t init_end = init_start;
    while (CUR(p) != CUP_TOK_EOF) {
        int k = CUR(p);
        if (depth == 0 && (k == ',' || k == ';' || k == '}')) break;
        if (k == '{' || k == '(' || k == '[') depth++;
        else if (k == '}' || k == ')' || k == ']') depth--;
        init_end = p->lex->cur.start_pos + 1;  /* 粗略估计当前位置之后 */
        NEXT(p);
    }

    /* 提取原始源码文本 */
    size_t init_len = 0;
    if (init_end > init_start) {
        /* 找到 = 之后的位置作为真正的起始 */
        const char *eq_pos = NULL;
        /* 从 init_start 向后搜索 = 符号 */
        for (size_t i = init_start; i < init_end && i < p->lex->src_len; i++) {
            if (p->lex->src[i] == '=') {
                eq_pos = p->lex->src + i + 1;
                break;
            }
            /* 遇到 ; 或 , 提前终止（无初始化值） */
            if (p->lex->src[i] == ';' || p->lex->src[i] == ',') break;
        }
        if (eq_pos) {
            /* 从 = 后面到当前 token 之前 */
            size_t eq_offset = eq_pos - p->lex->src;
            /* 找到当前 token 的起始位置作为结束 */
            size_t cur_start = p->lex->cur.start_pos;
            if (cur_start > eq_offset) {
                init_len = cur_start - eq_offset;
                /* 去除尾部空白 */
                while (init_len > 0 && isspace((unsigned char)p->lex->src[eq_offset + init_len - 1])) init_len--;
                /* 去除头部空白 */
                while (init_len > 0 && isspace((unsigned char)p->lex->src[eq_offset])) { eq_offset++; init_len--; }
                char *result = (char *)malloc(init_len + 1);
                if (!result) return strdup("");
                memcpy(result, p->lex->src + eq_offset, init_len);
                result[init_len] = '\0';
                return result;
            }
        }
    }

    return NULL;  /* 无初始值 */
}

/* ------------------------------------------------------------------ */
/* 顶层声明解析                                                        */
/* ------------------------------------------------------------------ */

/* 解析 struct/union/enum 的成员定义，当前 token 应为 {。
 * 成员填入 decl->members。结束后 cur 指向 } 之后。 */
static void cup_parse_struct_members(CuParser *p, CUPDecl *decl)
{
    if (CUR(p) != '{') return;
    NEXT(p);  /* 消费 { */

    while (CUR(p) != '}' && CUR(p) != CUP_TOK_EOF) {
        /* 解析一个成员：type + declarator (, declarator)* ; */
        SB mtype; sb_init(&mtype);
        char *tag = NULL;
        char tkind = cup_parse_type_specifier(p, &mtype, &tag, NULL, NULL, NULL, NULL);
        (void)tkind;
        if (tag) free(tag);

        /* 若类型说明符后紧跟 {，说明是嵌套的 struct/union/enum 定义
         * （如 struct Inner { ... } member;）。
         * 需要先跳过 { ... } 体，再解析 declarator。 */
        if (CUR(p) == '{') {
            cup_skip_balanced(p);
        }

        /* 处理匿名 struct/union 成员 */
        if (CUR(p) == ';') {
            NEXT(p);
            sb_free(&mtype);
            continue;
        }

        /* 多个 declarator 共享同一 type */
        while (1) {
            SB dtype; sb_init(&dtype);
            sb_append(&dtype, mtype.buf ? mtype.buf : "");
            /* 仅在不是以 [ ( * 开头时才加空格（数组/函数/指针不需要空格） */
            char last_ch = dtype.len > 0 ? dtype.buf[dtype.len - 1] : '\0';
            if (dtype.len > 0 && last_ch != ' ' && last_ch != '*' &&
                last_ch != '[' && last_ch != '(') {
                sb_append_char(&dtype, ' ');
            }
            char *msuffix = NULL;
            char *mname = cup_parse_declarator(p, &dtype, NULL, NULL, NULL, NULL, &msuffix);
            sb_trim(&dtype);

            /* 位域：: N */
            char *mvalue = NULL;
            if (CUR(p) == ':') {
                NEXT(p);
                SB vb; sb_init(&vb);
                sb_append_token(&vb, p->lex);
                NEXT(p);
                sb_trim(&vb);
                mvalue = strdup(vb.buf ? vb.buf : "");
                sb_free(&vb);
            }

            /* 添加成员：用 cup_build_full_type 组合基础类型 + suffix */
            if (decl->member_count + 1 > decl->member_count /* always true */ ) {
                decl->members = (CUParMember *)realloc(decl->members,
                                                       sizeof(CUParMember) * (decl->member_count + 1));
            }
            CUParMember *m = &decl->members[decl->member_count++];
            m->type = cup_build_full_type(&dtype, msuffix);
            m->name = mname ? strdup(mname) : NULL;
            m->value = mvalue;

            if (mname) free(mname);
            if (msuffix) free(msuffix);
            sb_free(&dtype);

            if (CUR(p) == ',') { NEXT(p); continue; }
            break;
        }

        /* 成员声明必须以 ; 结束。若不是，则报告错误并尝试恢复。 */
        if (CUR(p) == ';') {
            NEXT(p);
        } else if (CUR(p) == '}') {
            /* 缺少 ; 但直接到了 }：报告错误但不跳过，让外层循环处理 } */
            cup_error(p, "struct/union 成员声明缺少 ';'");
        } else if (CUR(p) == CUP_TOK_EOF) {
            cup_error(p, "struct/union 体未闭合");
            sb_free(&mtype);
            return;
        } else {
            /* 当前 token 既不是 ; 也不是 }：可能是漏写了 ; 。
             * 报告错误并跳过到下一个 ; 或 } 以便恢复。 */
            cup_error(p, "struct/union 成员声明缺少 ';'");
            while (CUR(p) != ';' && CUR(p) != '}' && CUR(p) != CUP_TOK_EOF) {
                NEXT(p);
            }
            if (CUR(p) == ';') NEXT(p);
        }
        sb_free(&mtype);
    }

    if (CUR(p) == '}') {
        NEXT(p);
    } else {
        cup_error(p, "struct/union/enum 体未闭合");
    }
}

/* 解析 enum 成员，当前 token 应为 {。
 * 每个成员的 name 为枚举常量名，value 为可选的 = 表达式。 */
static void cup_parse_enum_members(CuParser *p, CUPDecl *decl)
{
    if (CUR(p) != '{') return;
    NEXT(p);

    while (CUR(p) != '}' && CUR(p) != CUP_TOK_EOF) {
        if (CUR(p) != CUP_TOK_ID) {
            NEXT(p);
            continue;
        }
        char *ename = strdup(p->lex->cur.str);
        NEXT(p);

        char *evalue = NULL;
        if (CUR(p) == '=') {
            NEXT(p);
            evalue = cup_capture_initializer(p);
            /* capture 到 , 或 ;，需要回退一格：capture 已停在 , 或 ; */
        }

        decl->members = (CUParMember *)realloc(decl->members,
                                               sizeof(CUParMember) * (decl->member_count + 1));
        CUParMember *m = &decl->members[decl->member_count++];
        m->type = strdup("int");
        m->name = ename;
        m->value = evalue;

        if (CUR(p) == ',') NEXT(p);
        else if (CUR(p) == '}') break;
    }

    if (CUR(p) == '}') NEXT(p);
    else cup_error(p, "enum 体未闭合");
}

/* 解析预处理器指令：当前 cur.kind == CUP_TOK_PP，str 为指令名。
 * 读取该行剩余的 token，构造完整指令并加入结果。
 * 已在 #if 0 块中时，仅跟踪 #if/#else/#endif 嵌套。 */
static void cup_parse_preprocessor(CuParser *p)
{
    /* lex->cur.str 是指令名（如 "define", "include", "if" 等） */
    char *directive = p->lex->cur.str ? strdup(p->lex->cur.str) : strdup("");
    int line = p->lex->cur.line;

    /* 收集本行剩余 token */
    SB rest; sb_init(&rest);
    while (CUR(p) != CUP_TOK_EOF) {
        /* 预处理器指令在物理行末结束（除非有 \ 续行）。
         * 简化：只要下一个 token 的行号与当前不同，就结束。 */
        int prev_line = p->lex->cur.line;
        NEXT(p);
        if (CUR(p) == CUP_TOK_EOF) break;
        if (p->lex->cur.line != prev_line && p->lex->cur.kind != CUP_TOK_PP) {
            /* 行号变了，且不是新的 # 指令：说明本行结束 */
            /* 但是 lexer 没有 NEWLINE token，这里用行号变化判断 */
            break;
        }
        if (CUR(p) == CUP_TOK_PP) {
            /* 下一行又是 #，本行结束 */
            break;
        }
        sb_append_token(&rest, p->lex);
        sb_append_char(&rest, ' ');
    }
    sb_trim(&rest);

    /* 处理 #if 0 跳过 */
    if (strcmp(directive, "if") == 0) {
        /* 简化：若条件为 0，进入跳过模式 */
        if (strcmp(rest.buf ? rest.buf : "", "0") == 0) {
            p->pp_skip_depth++;
        }
    } else if (strcmp(directive, "ifdef") == 0 || strcmp(directive, "ifndef") == 0) {
        /* 简化：不跳过 */
    } else if (strcmp(directive, "else") == 0) {
        /* 简化：不处理 */
    } else if (strcmp(directive, "elif") == 0) {
        /* 简化：不处理 */
    } else if (strcmp(directive, "endif") == 0) {
        if (p->pp_skip_depth > 0) p->pp_skip_depth--;
    }

    if (p->pp_skip_depth > 0) {
        /* 跳过模式：不记录任何声明 */
        free(directive);
        sb_free(&rest);
        return;
    }

    /* 处理 #define */
    if (strcmp(directive, "define") == 0) {
        /* 宏名 = 第一个 token，剩余为宏值 */
        /* rest 形如 "NAME (a, b) value..." 或 "NAME value..." */
        char *macro_name = NULL;
        char *macro_value = NULL;
        char *s = rest.buf ? rest.buf : "";
        /* 提取第一个标识符 */
        int i = 0;
        while (s[i] && (isalpha((unsigned char)s[i]) || s[i] == '_' || isdigit((unsigned char)s[i]))) {
            i++;
        }
        if (i > 0) {
            macro_name = strndup(s, i);
        } else {
            cup_error(p, "#define 缺少宏名");
            free(directive);
            sb_free(&rest);
            return;
        }
        /* 跳过空白 */
        int j = i;
        while (s[j] == ' ' || s[j] == '\t') j++;
        /* 若紧跟 (：函数式宏 */
        if (s[j] == '(') {
            /* 包含参数列表到 value 中 */
            macro_value = strdup(s + i);
        } else if (s[j]) {
            macro_value = strdup(s + j);
        } else {
            macro_value = strdup("");
        }

        if (macro_name) {
            CUPDecl *d = cupdate_result_add_decl(p->result);
            d->kind = CUP_DECL_MACRO;
            d->name = macro_name;
            d->value = macro_value;
            d->line = line;
            d->is_api = 1;  /* 宏默认 API（用户可后续调整） */
        } else {
            free(macro_value);
        }
    }
    /* 处理 #include */
    else if (strcmp(directive, "include") == 0) {
        CUPDecl *d = cupdate_result_add_decl(p->result);
        d->kind = CUP_DECL_INCLUDE;
        d->name = strdup(rest.buf ? rest.buf : "");
        d->line = line;
    }
    /* 其他指令：忽略 */
    /* #undef：可选记录 */

    free(directive);
    sb_free(&rest);
}

/* 解析 typedef 声明：typedef <type> <declarator> [, <declarator>]* ;
 * 对于 typedef struct/union/enum [tag] { ... } declarator ; 形式，
 * 先消费 { ... } 体（记录为 struct/enum 定义），再解析 declarator。 */
static void cup_parse_typedef_decl(CuParser *p, SB *base_type_sb,
                                    int is_static, int is_inline)
{
    /* 若当前 token 是 {，说明是 typedef struct/union/enum [tag] { ... } ... 形式。
     * 先消费 struct body 并记录为一个 struct/enum 定义声明。 */
    if (CUR(p) == '{') {
        /* 从 base_type_sb 中提取 kind 和 tag 信息 */
        const char *s = base_type_sb->buf ? base_type_sb->buf : "";
        char kind = 's';  /* 默认 struct */
        if (strstr(s, "union")) kind = 'u';
        else if (strstr(s, "enum")) kind = 'e';

        /* 提取 tag：struct/union/enum 后面的标识符 */
        char *tag = NULL;
        const char *kw = strstr(s, kind == 'u' ? "union" :
                                   kind == 'e' ? "enum"  : "struct");
        if (kw) {
            kw += (kind == 'u') ? 5 : (kind == 'e') ? 4 : 6;
            while (*kw == ' ' || *kw == '\t') kw++;
            if (*kw && (isalpha((unsigned char)*kw) || *kw == '_')) {
                const char *start = kw;
                while (*kw && (isalnum((unsigned char)*kw) || *kw == '_')) kw++;
                tag = strndup(start, kw - start);
            }
        }

        CUPDecl *struct_decl = cupdate_result_add_decl(p->result);
        struct_decl->kind = (kind == 'e') ? CUP_DECL_ENUM : CUP_DECL_STRUCT;
        struct_decl->name = tag ? tag : NULL;
        struct_decl->line = p->lex->cur.line;
        struct_decl->is_static = is_static;
        struct_decl->is_inline = is_inline;
        struct_decl->is_api = !is_static;

        if (kind == 'e') {
            cup_parse_enum_members(p, struct_decl);
        } else {
            cup_parse_struct_members(p, struct_decl);
        }
    }

    /* 解析每个 declarator */
    while (1) {
        SB dtype; sb_init(&dtype);
        sb_append(&dtype, base_type_sb->buf ? base_type_sb->buf : "");
        char last_ch = dtype.len > 0 ? dtype.buf[dtype.len - 1] : '\0';
        if (dtype.len > 0 && last_ch != ' ' && last_ch != '*' &&
            last_ch != '[' && last_ch != '(') {
            sb_append_char(&dtype, ' ');
        }

        int is_func = 0;
        int is_func_def = 0;
        CUParParam *params = NULL;
        int pcount = 0;
        char *tsuffix = NULL;
        char *name = cup_parse_declarator(p, &dtype, &params, &pcount,
                                          &is_func, &is_func_def, &tsuffix);
        sb_trim(&dtype);

        if (name) {
            CUPDecl *d = cupdate_result_add_decl(p->result);
            d->kind = CUP_DECL_TYPEDEF;
            d->name = strdup(name);
            d->base_type = cup_build_full_type(&dtype, tsuffix);
            if (tsuffix) free(tsuffix);
            d->line = p->lex->cur.line;
            d->is_static = is_static;
            d->is_inline = is_inline;
            d->is_api = !is_static;
            (void)params; (void)pcount; (void)is_func; (void)is_func_def;
            /* typedef 不收集 params（简化） */
            if (params) {
                cup_free_param_array(params, pcount);
            }
            free(name);
        } else {
            /* 匿名 typedef：可能是 `typedef int;` 这类语法错误，
             * 也可能是合法的 `typedef struct { ... };` （此时 struct_decl
             * 已单独记录）。对前者发出警告。 */
            if (CUR(p) == ';' && !is_func) {
                cup_warning(p, "typedef 缺少声明名（无 declarator）");
            }
            if (params) {
                cup_free_param_array(params, pcount);
            }
        }
        sb_free(&dtype);

        if (CUR(p) == ',') { NEXT(p); continue; }
        break;
    }

    /* 消费 ; */
    if (CUR(p) == ';') {
        NEXT(p);
    } else {
        /* 跳过到 ; */
        cup_skip_to(p, ';');
        if (CUR(p) == ';') NEXT(p);
    }
}

/* 解析 struct/union/enum 声明：
 * 形如：struct [tag] { ... } [declarator (, declarator)*] ;
 * 或：struct tag declarator (, declarator)* ;
 * 当前 cur 应位于 { 或 declarator 之前。base_type_sb 已包含 "struct [tag]"。
 * kind 为 's' / 'u' / 'e'。tag 可能为 NULL。
 */
static void cup_parse_struct_decl(CuParser *p, SB *base_type_sb, char kind,
                                   char *tag, int is_static, int is_inline)
{
    CUPDecl *struct_decl = NULL;

    /* 若有 { ... }：定义 */
    if (CUR(p) == '{') {
        struct_decl = cupdate_result_add_decl(p->result);
        struct_decl->kind = (kind == 'e') ? CUP_DECL_ENUM : CUP_DECL_STRUCT;
        struct_decl->name = tag ? strdup(tag) : NULL;
        struct_decl->line = p->lex->cur.line;
        struct_decl->is_static = is_static;
        struct_decl->is_inline = is_inline;
        struct_decl->is_api = !is_static;

        if (kind == 'e') {
            cup_parse_enum_members(p, struct_decl);
        } else {
            cup_parse_struct_members(p, struct_decl);
        }

        /* 若 struct 有 tag 但紧跟的是变量声明，则下方 declarator 处理 */
        /* 否则可能是 struct 定义后立即 ; */
    }

    /* 后续 declarator（变量声明，使用此 struct 类型） */
    /* 若当前是 ; 且 struct 已定义：纯 struct 定义，结束 */
    if (CUR(p) == ';') {
        NEXT(p);
        if (tag) free(tag);
        return;
    }

    /* 可能是 typedef 或变量声明 */
    if (CUR(p) == CUP_TOK_ID || CUR(p) == '*') {
        /* 多个 declarator */
        while (1) {
            SB dtype; sb_init(&dtype);
            sb_append(&dtype, base_type_sb->buf ? base_type_sb->buf : "");
            char last_ch = dtype.len > 0 ? dtype.buf[dtype.len - 1] : '\0';
            if (dtype.len > 0 && last_ch != ' ' && last_ch != '*' &&
                last_ch != '[' && last_ch != '(') {
                sb_append_char(&dtype, ' ');
            }
            int is_func = 0, is_func_def = 0;
            char *vsuffix = NULL;
            char *vname = cup_parse_declarator(p, &dtype, NULL, NULL,
                                               &is_func, &is_func_def, &vsuffix);
            sb_trim(&dtype);
            (void)is_func; (void)is_func_def;

            if (vname) {
                CUPDecl *d = cupdate_result_add_decl(p->result);
                d->kind = CUP_DECL_VARIABLE;
                d->name = strdup(vname);
                d->base_type = cup_build_full_type(&dtype, vsuffix);
                if (vsuffix) free(vsuffix);
                d->line = p->lex->cur.line;
                d->is_static = is_static;
                d->is_api = !is_static;
                free(vname);
            }
            sb_free(&dtype);

            if (CUR(p) == ',') { NEXT(p); continue; }
            break;
        }
    }

    if (CUR(p) == ';') {
        NEXT(p);
    } else {
        cup_skip_to(p, ';');
        if (CUR(p) == ';') NEXT(p);
    }
    if (tag) free(tag);
}

/* 检查标识符是否是调用约定关键字 */
static int cup_is_calling_convention(const char *id)
{
    if (!id) return 0;
    static const char *cc_list[] = {
        "__cdecl", "__stdcall", "__fastcall", "__thiscall",
        "__naked", "__nakedcall", "__pascal", "__forceinline",
        "WINAPI", "APIENTRY", "CALLBACK", "WIN calling convention",
        NULL
    };
    for (int i = 0; cc_list[i]; i++) {
        if (strcmp(id, cc_list[i]) == 0) return 1;
    }
    return 0;
}

/* 尝试提取调用约定：如果当前 token 是调用约定标识符则消费并返回 */
static char *cup_try_extract_calling_convention(CuParser *p)
{
    if (CUR(p) == CUP_TOK_ID && cup_is_calling_convention(p->lex->cur.str)) {
        char *cc = strdup(p->lex->cur.str);
        NEXT(p);
        /* 有些调用约定可能是两个标记组成，如 __declspec(naked)，
         * 这里简化处理，仅支持单个标识符形式 */
        return cc;
    }
    return NULL;
}

/* 解析函数或变量声明（非 typedef, 非 struct/union/enum 定义）。
 * base_type_sb 已含基础类型。 */
static void cup_parse_function_or_var(CuParser *p, SB *base_type_sb,
                                       int is_static, int is_inline, int is_extern)
{
    (void)is_extern;
    int processed_func_def = 0;  /* 标记是否处理了函数定义（有函数体） */
    while (1) {
        SB dtype; sb_init(&dtype);
        sb_append(&dtype, base_type_sb->buf ? base_type_sb->buf : "");
        char last_ch = dtype.len > 0 ? dtype.buf[dtype.len - 1] : '\0';
        if (dtype.len > 0 && last_ch != ' ' && last_ch != '*' &&
            last_ch != '[' && last_ch != '(') {
            sb_append_char(&dtype, ' ');
        }

        /* 尝试提取调用约定（在类型和函数名之间） */
        char *call_conv = cup_try_extract_calling_convention(p);

        int is_func = 0, is_func_def = 0;
        CUParParam *params = NULL;
        int pcount = 0;
        char *fsuffix = NULL;
        char *name = cup_parse_declarator(p, &dtype, &params, &pcount,
                                          &is_func, &is_func_def, &fsuffix);
        sb_trim(&dtype);

        if (name && is_func) {
            /* 函数定义或原型 */
            CUPDecl *d = cupdate_result_add_decl(p->result);
            d->kind = CUP_DECL_FUNCTION;
            d->name = strdup(name);
            /* return_type: 仅基础类型（不含函数参数后缀） */
            d->return_type = strdup(dtype.buf ? dtype.buf : "");
            d->call = call_conv;  /* 存储调用约定 */
            d->params = params;
            d->param_count = pcount;
            d->is_function_def = is_func_def;
            d->is_static = is_static;
            d->is_inline = is_inline;
            d->is_api = !is_static;
            d->line = p->lex->cur.line;

            if (is_func_def && CUR(p) == '{') {
                d->body = cup_capture_braced_body(p);
                /* 函数定义后通常 ; 可选 */
                if (CUR(p) == ';') NEXT(p);
            } else {
                /* 函数原型：; 结束 */
                if (CUR(p) == ';') NEXT(p);
                else {
                    cup_skip_to(p, ';');
                    if (CUR(p) == ';') NEXT(p);
                }
            }
            processed_func_def = 1;  /* 函数（原型或定义）均已处理完毕，无需再消费 ; */
            free(name);
            sb_free(&dtype);
            break;  /* 函数定义/原型后不能有多个 declarator */
        }

        /* 变量 */
        if (name) {
            CUPDecl *d = cupdate_result_add_decl(p->result);
            d->kind = CUP_DECL_VARIABLE;
            d->name = strdup(name);
            /* 用 cup_build_full_type 组合基础类型 + suffix（如 [10]） */
            d->base_type = cup_build_full_type(&dtype, fsuffix);
            if (fsuffix) free(fsuffix);
            d->is_static = is_static;
            d->is_inline = is_inline;
            d->is_api = !is_static;
            d->line = p->lex->cur.line;

            /* 初始值 */
            if (CUR(p) == '=') {
                NEXT(p);
                d->value = cup_capture_initializer(p);
            }
            free(name);
        }

        /* 释放 params（变量不应有 params，但安全起见） */
        if (params) {
            cup_free_param_array(params, pcount);
        }
        sb_free(&dtype);

        if (CUR(p) == ',') { NEXT(p); continue; }
        break;
    }

    /* 函数定义（有函数体）不需要再消费 ; */
    if (processed_func_def) return;

    /* 消费可能未被消费的 ;（变量声明场景）。
     * 函数原型/定义的 ; 已在 while 循环内消费完毕。 */
    if (CUR(p) == ';') {
        NEXT(p);
    } else if (CUR(p) != CUP_TOK_EOF && CUR(p) != '}' && CUR(p) != '{') {
        /* 既不是 ; 也不是 EOF/}，说明缺少分号 */
        char buf[256];
        snprintf(buf, sizeof(buf), "缺少分号 ';'（当前 token: '%s'）",
                 p->lex->cur.str ? p->lex->cur.str : "<non-id>");
        cup_warning(p, buf);
    }
}

/* ------------------------------------------------------------------ */
/* 主解析循环                                                          */
/* ------------------------------------------------------------------ */

static void cup_parse_top_level(CuParser *p)
{
    while (CUR(p) != CUP_TOK_EOF) {
        int k = CUR(p);

        /* 预处理器指令 */
        if (k == CUP_TOK_PP) {
            cup_parse_preprocessor(p);
            continue;
        }

        /* 空语句 */
        if (k == ';') {
            NEXT(p);
            continue;
        }

        /* _Static_assert(...) ; */
        if (k == CUP_TOK__STATIC_ASSERT) {
            NEXT(p);
            if (CUR(p) == '(') cup_skip_balanced(p);
            if (CUR(p) == ';') NEXT(p);
            continue;
        }

        /* asm(...) ; (GCC 扩展) */
        if (k == CUP_TOK_ID && p->lex->cur.str &&
            (strcmp(p->lex->cur.str, "__asm") == 0 ||
             strcmp(p->lex->cur.str, "asm") == 0 ||
             strcmp(p->lex->cur.str, "__attribute__") == 0)) {
            /* 跳过属性或 asm 块 */
            while (CUR(p) == CUP_TOK_ID && p->lex->cur.str &&
                   (strcmp(p->lex->cur.str, "__asm") == 0 ||
                    strcmp(p->lex->cur.str, "asm") == 0 ||
                    strcmp(p->lex->cur.str, "__attribute__") == 0)) {
                NEXT(p);
                while (CUR(p) == '(') cup_skip_balanced(p);
            }
            if (CUR(p) == ';') NEXT(p);
            continue;
        }

        /* 解析类型说明符 */
        SB base_type; sb_init(&base_type);
        char *tag = NULL;
        int is_static = 0, is_inline = 0, is_extern = 0, is_typedef = 0;
        char tkind = cup_parse_type_specifier(p, &base_type, &tag,
                                              &is_static, &is_inline, &is_extern,
                                              &is_typedef);

        if (tkind == 0) {
            /* 无法识别类型：可能是语法错误，跳过到 ; 或 } */
            if (CUR(p) == CUP_TOK_EOF) {
                sb_free(&base_type);
                if (tag) free(tag);
                break;
            }
            /* 报告错误并恢复 */
            if (CUR(p) != ';' && CUR(p) != '{' && CUR(p) != '}') {
                char buf[256];
                snprintf(buf, sizeof(buf), "无法识别的声明（开头 token: '%s'）",
                         p->lex->cur.str ? p->lex->cur.str : "<non-id>");
                cup_error(p, buf);
            }
            /* 跳过到 ; 并继续 */
            cup_skip_to(p, ';');
            if (CUR(p) == ';') NEXT(p);
            sb_free(&base_type);
            if (tag) free(tag);
            continue;
        }

        /* 检查是否是 typedef */
        if (is_typedef) {
            cup_parse_typedef_decl(p, &base_type, is_static, is_inline);
            sb_free(&base_type);
            if (tag) free(tag);
            continue;
        }

        /* struct/union/enum 定义或变量声明 */
        if (tkind == 's' || tkind == 'u' || tkind == 'e') {
            if (CUR(p) == '{') {
                /* struct/union/enum 定义 */
                cup_parse_struct_decl(p, &base_type, tkind, tag,
                                      is_static, is_inline);
                sb_free(&base_type);
                continue;
            }
            /* struct tag 变量声明：fall through 到普通函数/变量解析 */
            /* 但 tag 已被消费到 base_type_sb，cup_parse_struct_decl 也处理此情况 */
            cup_parse_struct_decl(p, &base_type, tkind, tag,
                                  is_static, is_inline);
            sb_free(&base_type);
            continue;
        }

        /* 普通函数或变量 */
        cup_parse_function_or_var(p, &base_type, is_static, is_inline, is_extern);
        sb_free(&base_type);
        if (tag) free(tag);
    }
}

/* ------------------------------------------------------------------ */
/* 公共入口                                                            */
/* ------------------------------------------------------------------ */

/* 检查重复定义：同一 kind 下不允许同名 */
static void cup_check_duplicates(CUPResult *r)
{
    for (int i = 0; i < r->decl_count; i++) {
        CUPDecl *di = &r->decls[i];
        if (!di->name || di->kind == CUP_DECL_INCLUDE || di->kind == CUP_DECL_OTHER) continue;

        for (int j = i + 1; j < r->decl_count; j++) {
            CUPDecl *dj = &r->decls[j];
            if (!dj->name || dj->kind == CUP_DECL_INCLUDE || dj->kind == CUP_DECL_OTHER) continue;

            if (di->kind == dj->kind && strcmp(di->name, dj->name) == 0) {
                /* 函数：只有两个都是定义（is_function_def=1）才算重复；
                 * 多个原型（前向声明）在 C 中合法，不报重复 */
                if (di->kind == CUP_DECL_FUNCTION &&
                    !(di->is_function_def && dj->is_function_def))
                    continue;
                char buf[256];
                snprintf(buf, sizeof(buf), "重复定义: '%s' 已在第 %d 行定义",
                         di->name, di->line);
                cupdate_result_add_error(r, buf, dj->line);
            }
        }
    }
}

int cup_parse(CUPResult *r, const char *source, const char *filename)
{
    if (!r || !source) return -1;
    CuLexer lex;
    cu_lex_init(&lex, source, filename, r);

    CuParser p;
    p.lex = &lex;
    p.result = r;
    p.filename = filename;
    p.pp_skip_depth = 0;

    cup_parse_top_level(&p);

    /* 后处理：检测重复定义 */
    cup_check_duplicates(r);

    cu_lex_free(&lex);
    return 0;
}
