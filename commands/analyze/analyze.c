/* analyze.c - CBoot generated (compiler: normal) */
/* Module: commands/analyze */
/*
 * analyze 子模块：代码分析实现
 * 父模块 commands 仅做入口转发，所有分析实现放此子模块。
 * 设计原则：
 *   父模块放函数调用（调度），子模块放具体实现。
 *   高依赖图复杂度的函数也可进一步下沉到子模块。
 */

#include "cboot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#define ANALYZE_NGRAM_SIZE 8
#define ANALYZE_MAX_MODS   256
#define ANALYZE_MAX_FUNCS  2048
#define ANALYZE_MAX_TOKENS 8192

typedef struct {
    char name[128];
    const char *code;       /* 指向 ModuleDomain.code，不持有所有权 */
    int  code_lines;
} AnalyzeMod;

typedef struct {
    char name[128];
    char module[128];
    char *code;             /* 函数体，持有所有权 (strdup) */
    int  complexity;
    int  call_count;        /* 被其他函数调用的次数（依赖图复杂度 = call_count - 1） */
} AnalyzeFunc;

typedef struct {
    char text[64];          /* 标识符保留原文；数字/字符串/字符归一化 */
} AnalyzeToken;

/* 构造模块路径名：从根走到当前模块，用 "/" 连接 */
static void analyze_build_module_path(Domain *d, char *out, size_t size) {
    out[0] = '\0';
    char parts[16][128];
    int n = 0;
    Domain *p = d;
    while (p && p->type == DOMAIN_MODULE && n < 16) {
        if (p->parent == NULL) break;  /* 跳过根项目 */
        if (p->name) {
            strncpy(parts[n], p->name, 127);
            parts[n][127] = '\0';
            n++;
        }
        p = p->parent;
    }
    for (int i = n - 1; i >= 0; i--) {
        if (out[0] != '\0') strncat(out, "/", size - strlen(out) - 1);
        strncat(out, parts[i], size - strlen(out) - 1);
    }
}

static void analyze_collect_modules(Domain *d, AnalyzeMod *mods, int *count, int cap)
{
    if (!d || *count >= cap) return;

    if (d->type == DOMAIN_MODULE) {
        ModuleDomain *md = (ModuleDomain *)d;
        if (md->mode == MOD_MODE_SRC && md->code && md->code[0] != '\0') {
            AnalyzeMod *m = &mods[*count];
            analyze_build_module_path(d, m->name, sizeof(m->name));
            m->code = md->code;
            m->code_lines = 0;
            (*count)++;
        }
    }

    for (int i = 0; i < d->child_count; i++)
        analyze_collect_modules(d->children[i], mods, count, cap);
}

/* 处理行尾：若当前行已有代码字符则计数，并重置标记 */
static void analyze_count_line_end(int *has_code, int *lines) {
    if (*has_code) (*lines)++;
    *has_code = 0;
}

/* ST_NORMAL 状态下处理字符；返回新状态 */
static int analyze_count_normal_char(char c, const char *p, const char **next,
                                     int *has_code) {
    if (c == '/' && p[1] == '/') { *next = p + 1; return 1 /*ST_LINE_CMT*/; }
    if (c == '/' && p[1] == '*') { *next = p + 1; return 2 /*ST_BLOCK_CMT*/; }
    if (c == '"')  { *has_code = 1; return 3 /*ST_STRING*/; }
    if (c == '\'') { *has_code = 1; return 4 /*ST_CHAR*/; }
    if (!isspace((unsigned char)c)) *has_code = 1;
    return 0;
}

/* 处理引号状态（STRING/CHAR 逻辑相同，仅 quote 字符不同） */
static int analyze_count_quote_char(char c, char quote, const char **next) {
    if (c == '\\') { *next = *next + 1; return 1; }
    if (c == quote) return 0;  /* 退出引号状态 */
    return 1;  /* 保持引号状态 */
}

/* 状态机上下文，避免长参数列表 */
typedef struct {
    int lines;
    int has_code;
    int state;     /* 0=NORMAL 1=LINE_CMT 2=BLOCK_CMT 3=QUOTE */
    char quote;    /* 当前引号字符（仅 state==3 有效） */
} CountCtx;

/* ST_NORMAL: 处理普通字符，返回新状态 */
static int count_st_normal(char c, const char *p, const char **next, CountCtx *ctx) {
    if (c == '\n') { analyze_count_line_end(&ctx->has_code, &ctx->lines); return 0; }
    int s = analyze_count_normal_char(c, p, next, &ctx->has_code);
    if (s == 3) { ctx->quote = '"';  return 3; }
    if (s == 4) { ctx->quote = '\''; return 3; }
    return s;
}

/* ST_LINE_CMT: 行注释中遇到换行则结束 */
static int count_st_line_cmt(char c, CountCtx *ctx) {
    if (c == '\n') { analyze_count_line_end(&ctx->has_code, &ctx->lines); return 0; }
    return 1;
}

/* ST_BLOCK_CMT: 块注释中遇到 star-slash 则结束 */
static int count_st_block_cmt(char c, const char *p, const char **next, CountCtx *ctx) {
    if (c == '*' && p[1] == '/') { *next = p + 1; return 0; }
    if (c == '\n') analyze_count_line_end(&ctx->has_code, &ctx->lines);
    return 2;
}

/* ST_QUOTE: 字符串/字符字面量，遇到匹配引号则结束 */
static int count_st_quote(char c, const char **next, CountCtx *ctx) {
    if (!analyze_count_quote_char(c, ctx->quote, next)) return 0;
    if (c == '\n') analyze_count_line_end(&ctx->has_code, &ctx->lines);
    return 3;
}

static int analyze_count_code_lines(const char *code)
{
    if (!code) return 0;
    CountCtx ctx = {0, 0, 0, 0};
    const char *p = code;
    while (*p) {
        char c = *p;
        const char *next = p;
        switch (ctx.state) {
        case 0: ctx.state = count_st_normal(c, p, &next, &ctx); break;
        case 1: ctx.state = count_st_line_cmt(c, &ctx); break;
        case 2: ctx.state = count_st_block_cmt(c, p, &next, &ctx); break;
        case 3: ctx.state = count_st_quote(c, &next, &ctx); break;
        }
        p = (next > p) ? next + 1 : p + 1;
    }
    if (ctx.has_code) ctx.lines++;
    return ctx.lines;
}

/* 从源码中提取函数：匹配 "type name(...)" 后跟 "{" 的模式 */
/* 检查名称是否为控制流关键字 */
static int analyze_is_keyword(const char *name, int len) {
    static const char *keywords[] = {"if", "for", "while", "switch", "return", "sizeof", "else", "do", 0};
    for (int k = 0; keywords[k]; k++) {
        if (len == (int)strlen(keywords[k]) && strncmp(name, keywords[k], len) == 0)
            return 1;
    }
    return 0;
}

/* 跳过字符串内容，返回跳过后的指针 */
static const char *analyze_skip_string(const char *p, char quote);
/* 跳过字符串、字符、注释、预处理指令 */
static const char *analyze_skip_non_code(const char *p);

/* 查找函数体结束位置（匹配大括号），跳过字符串、字符和注释
 * （注释中的 } 不得误计为大括号闭合） */
static const char *analyze_find_body_end(const char *body_start) {
    const char *p = body_start;
    int brace_depth = 1;
    while (*p && brace_depth > 0) {
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }
        if (*p == '{') brace_depth++;
        else if (*p == '}') brace_depth--;
        if (p < body_start) break;
        p++;
    }
    return (brace_depth == 0) ? p : NULL;
}

/* 跳过字符串、字符、注释、预处理指令 */
/* 跳过行注释或预处理指令（读到换行） */
static const char *analyze_skip_line_comment(const char *p) {
    while (*p && *p != '\n') p++;
    return p;
}

/* 跳过块注释 */
static const char *analyze_skip_block_comment(const char *p) {
    p += 2;
    while (*p && !(p[0] == '*' && p[1] == '/')) p++;
    if (*p) p += 2;
    return p;
}

static const char *analyze_skip_non_code(const char *p) {
    if (*p == '"' || *p == '\'') return analyze_skip_string(p, *p);
    if (p[0] == '/' && p[1] == '/') return analyze_skip_line_comment(p);
    if (*p == '#') return analyze_skip_line_comment(p);
    if (p[0] == '/' && p[1] == '*') return analyze_skip_block_comment(p);
    return NULL;
}

/* 在 [start, ...) 范围内查找配对的圆括号；成功返回 1 并设置 open/close 指针，
 * 失败时返回 0。查找到 '{' 或 ';' 或 '\0' 即停止。
 * 跳过注释、字符串、预处理指令，避免注释中的 name( 被误识别为函数调用。 */
static int analyze_find_parens(const char *start,
                               const char **open_paren, const char **close_paren) {
    const char *paren = start;
    int depth = 0;
    *open_paren = NULL;
    *close_paren = NULL;
    while (*paren && *paren != '{' && *paren != ';') {
        const char *skipped = analyze_skip_non_code(paren);
        if (skipped) { paren = skipped; continue; }
        if (*paren == '(') {
            if (depth == 0) *open_paren = paren;
            depth++;
        }
        if (*paren == ')') {
            depth--;
            if (depth == 0 && *open_paren) *close_paren = paren;
        }
        paren++;
    }
    return (depth == 0 && *paren == '{' && *open_paren && *close_paren) ? 1 : 0;
}

/* 从 open_paren 回溯到 start，提取函数名 [name_start, name_end)，并校验存在返回类型 */
static int analyze_extract_func_name(const char *start, const char *open_paren,
                                     const char **name_start, const char **name_end) {
    const char *ne = open_paren;
    while (ne > start && isspace((unsigned char)ne[-1])) ne--;
    const char *ns = ne;
    while (ns > start && (isalnum((unsigned char)ns[-1]) || ns[-1] == '_')) ns--;
    int has_type = 0;
    for (const char *c = start; c < ns; c++) {
        if (!isspace((unsigned char)*c)) { has_type = 1; break; }
    }
    *name_start = ns;
    *name_end = ne;
    return (has_type && ns < ne) ? 1 : 0;
}

/* 尝试在当前行起始位置解析一个函数定义；成功返回 1，*next 指向函数体之后 */
static int analyze_try_extract_function(const char *start,
                                        const char *mod_name,
                                        AnalyzeFunc *funcs, int *count, int cap,
                                        const char **next) {
    /* 跳过行首空白（isspace('\0') 返回 0，循环自然终止） */
    while (isspace((unsigned char)*start)) start++;
    if (*start == '\0') return 0;

    const char *open_paren, *close_paren;
    if (!analyze_find_parens(start, &open_paren, &close_paren)) return 0;
    const char *brace = close_paren;
    while (*brace && *brace != '{') brace++;
    if (*brace != '{') return 0;

    const char *name_start, *name_end;
    if (!analyze_extract_func_name(start, open_paren, &name_start, &name_end)) return 0;

    int nlen = (int)(name_end - name_start);
    if (nlen <= 0 || nlen >= 127 || analyze_is_keyword(name_start, nlen)) return 0;

    const char *body_start = brace + 1;
    const char *body_end = analyze_find_body_end(body_start);
    if (!body_end) return 0;
    if (*count >= cap) return 0;

    AnalyzeFunc *f = &funcs[*count];
    strncpy(f->name, name_start, nlen);
    f->name[nlen] = '\0';
    strncpy(f->module, mod_name, 127);
    f->module[127] = '\0';
    int blen = (int)(body_end - body_start);
    f->code = (char *)malloc(blen + 1);
    if (f->code) {
        memcpy(f->code, body_start, blen);
        f->code[blen] = '\0';
        (*count)++;
    }
    *next = body_end;
    return 1;
}

static void analyze_extract_functions(const char *mod_name, const char *code,
                                       AnalyzeFunc *funcs, int *count, int cap)
{
    if (!code || !*code) return;
    const char *p = code;
    while (*p && *count < cap) {
        /* 跳过字符串、注释、预处理 */
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }

        /* 记录行首位置（可能为函数定义开头） */
        if (p == code || p[-1] == '\n') {
            const char *next = NULL;
            if (analyze_try_extract_function(p, mod_name, funcs, count, cap, &next)) {
                p = next;
                continue;
            }
        }
        p++;
    }
}

/* 简单词法分析：将代码拆分为 token，用于 n-gram 比对 */
/* 检查是否为双字符运算符 */
static int analyze_is_two_char_op(char a, char b) {
    static const char ops[][3] = {
        "==","!=","<=",">=","&&","||","++","--","->","<<",">>",
        "+=","-=","*=","/=","%=","&=","|=","^=",".."
    };
    for (int i = 0; i < (int)(sizeof(ops)/sizeof(ops[0])); i++) {
        if (a == ops[i][0] && b == ops[i][1]) return 1;
    }
    return 0;
}

/* 跳过字符串内容，返回跳过后的指针 */
static const char *analyze_skip_string(const char *p, char quote) {
    p++; /* skip opening quote */
    while (*p && *p != quote) {
        if (*p == '\\') p++;
        if (*p) p++;
    }
    if (*p) p++;
    return p;
}

/* 添加一个字面量归一化 token（"STR"/"'C'"/"NUM"） */
static int analyze_emit_literal(AnalyzeToken *toks, int *count, int cap, const char *text) {
    if (*count >= cap) return 0;
    strncpy(toks[*count].text, text, 63);
    toks[*count].text[63] = '\0';
    (*count)++;
    return 1;
}

/* 处理标识符 token；返回 1 表示已处理，0 表示当前字符不是标识符开头 */
static int analyze_emit_identifier(const char **p, AnalyzeToken *toks, int *count, int cap) {
    if (!isalpha((unsigned char)**p) && **p != '_') return 0;
    if (*count >= cap) return 0;
    int len = 0;
    while ((isalnum((unsigned char)**p) || **p == '_') && len < 63) {
        toks[*count].text[len++] = **p;
        (*p)++;
    }
    toks[*count].text[len] = '\0';
    while (isalnum((unsigned char)**p) || **p == '_') (*p)++;
    (*count)++;
    return 1;
}

/* 处理运算符和标点符号 token */
static int analyze_emit_operator(const char **p, AnalyzeToken *toks, int *count, int cap) {
    if (!ispunct((unsigned char)**p)) return 0;
    if (*count >= cap) return 0;
    if ((*p)[0] && (*p)[1] && analyze_is_two_char_op((*p)[0], (*p)[1])) {
        toks[*count].text[0] = (*p)[0];
        toks[*count].text[1] = (*p)[1];
        toks[*count].text[2] = '\0';
        *p += 2;
    } else {
        toks[*count].text[0] = **p;
        toks[*count].text[1] = '\0';
        (*p)++;
    }
    (*count)++;
    return 1;
}

/* 跳过空白和注释；返回 1 表示已跳过非代码内容，0 表示当前位置是代码 */
static int analyze_skip_trivia(const char **p) {
    if (isspace((unsigned char)**p)) { (*p)++; return 1; }
    if ((*p)[0] == '/' && (*p)[1] == '/') {
        while (**p && **p != '\n') (*p)++;
        return 1;
    }
    if ((*p)[0] == '/' && (*p)[1] == '*') {
        *p += 2;
        while (**p && !((*p)[0] == '*' && (*p)[1] == '/')) (*p)++;
        if (**p) *p += 2;
        return 1;
    }
    return 0;
}

/* 处理字面量 token（字符串/字符/数字）；返回 1 表示已处理 */
static int analyze_emit_literal_token(const char **p, AnalyzeToken *toks, int *count, int cap) {
    char ch = **p;
    if (ch == '"' || ch == '\'') {
        analyze_emit_literal(toks, count, cap, ch == '"' ? "\"STR\"" : "'C'");
        *p = analyze_skip_string(*p, ch);
        return 1;
    }
    if (isdigit((unsigned char)ch) || (ch == '.' && isdigit((unsigned char)(*p)[1]))) {
        analyze_emit_literal(toks, count, cap, "NUM");
        while (isdigit((unsigned char)**p) || **p == '.' || **p == 'x' || **p == 'X') (*p)++;
        return 1;
    }
    return 0;
}

static int analyze_tokenize(const char *code, AnalyzeToken *toks, int cap)
{
    int count = 0;
    const char *p = code;
    while (*p && count < cap) {
        if (analyze_skip_trivia(&p)) continue;
        if (analyze_emit_literal_token(&p, toks, &count, cap)) continue;
        if (analyze_emit_identifier(&p, toks, &count, cap)) continue;
        if (analyze_emit_operator(&p, toks, &count, cap))   continue;
        p++;
    }
    return count;
}

/* 计算圈复杂度：基于分支关键字和运算符计数 */
static int analyze_cyclomatic_complexity(const char *code)
{
    if (!code) return 1;
    int cc = 1;
    AnalyzeToken toks[ANALYZE_MAX_TOKENS];
    int n = analyze_tokenize(code, toks, ANALYZE_MAX_TOKENS);
    for (int i = 0; i < n; i++) {
        const char *t = toks[i].text;
        if (strcmp(t, "if") == 0 || strcmp(t, "else") == 0 ||
            strcmp(t, "for") == 0 || strcmp(t, "while") == 0 ||
            strcmp(t, "case") == 0 || strcmp(t, "catch") == 0 ||
            strcmp(t, "&&") == 0 || strcmp(t, "||") == 0 ||
            strcmp(t, "?") == 0) {
            cc++;
        }
    }
    return cc;
}

/* 比较两个 n-gram 是否相等 */
static int analyze_ngram_equal(AnalyzeToken *a, AnalyzeToken *b)
{
    for (int i = 0; i < ANALYZE_NGRAM_SIZE; i++) {
        if (strcmp(a[i].text, b[i].text) != 0) return 0;
    }
    return 1;
}

/* 常见固定语句模式：8-token 模板，NULL 表示通配符（匹配任意 token）。
 * 这些是 C 语言中固有重复的控制流/资源管理模板，
 * 多次出现属正常语法结构，不应计入"语义重复"统计。
 * 涵盖 for 循环计数器滑动窗口、错误检查、资源释放等。 */
typedef struct {
    const char *tok[ANALYZE_NGRAM_SIZE];
} AnalyzeNGramPattern;

static const AnalyzeNGramPattern analyze_common_patterns[] = {
    /* for ( int i = NUM ; i  -- 后接 < 或 <= */
    { {"for","(","int","i","=","NUM",";","i"} },
    { {"for","(","int","j","=","NUM",";","j"} },
    { {"for","(","int","k","=","NUM",";","k"} },
    /* ( int i = NUM ; i <  -- 滑动窗口 */
    { {"(","int","i","=","NUM",";","i","<"} },
    { {"(","int","j","=","NUM",";","j","<"} },
    { {"(","int","k","=","NUM",";","k","<"} },
    /* int i = NUM ; i < * */
    { {"int","i","=","NUM",";","i","<",NULL} },
    { {"int","j","=","NUM",";","j","<",NULL} },
    { {"int","k","=","NUM",";","k","<",NULL} },
    /* = NUM ; i < * ; i  -- 步进部分 */
    { {"=","NUM",";","i","<",NULL,";","i"} },
    { {"=","NUM",";","j","<",NULL,";","j"} },
    { {"=","NUM",";","k","<",NULL,";","k"} },
    /* NUM ; i < * ; i ++ */
    { {"NUM",";","i","<",NULL,";","i","++"} },
    { {"NUM",";","j","<",NULL,";","j","++"} },
    { {"NUM",";","k","<",NULL,";","k","++"} },
    /* ; i < * ; i ++ ) */
    { {";","i","<",NULL,";","i","++",")"} },
    { {";","j","<",NULL,";","j","++",")"} },
    { {";","k","<",NULL,";","k","++",")"} },
    /* i < * ; i ++ ) {  -- 循环体进入 */
    { {"i","<",NULL,";","i","++",")","{"} },
    { {"j","<",NULL,";","j","++",")","{"} },
    { {"k","<",NULL,";","k","++",")","{"} },
    /* < * ; i ++ ) { *  -- 循环体跨过比较值 */
    { {"<",NULL,";","i","++",")","{",NULL} },
    { {"<",NULL,";","j","++",")","{",NULL} },
    { {"<",NULL,";","k","++",")","{",NULL} },
    /* * ; i ++ ) { *  -- 跨过整个步进表达式 */
    { {NULL,";","i","++",")","{",NULL,NULL} },
    { {NULL,";","j","++",")","{",NULL,NULL} },
    { {NULL,";","k","++",")","{",NULL,NULL} },
    /* for ( i = NUM ; i <  -- 无 int 声明 */
    { {"for","(","i","=","NUM",";","i","<"} },
    { {"for","(","j","=","NUM",";","j","<"} },
    { {"for","(","k","=","NUM",";","k","<"} },
    /* i = NUM ; i < * ;  -- 滑动窗口（无 int 前缀） */
    { {"i","=","NUM",";","i","<",NULL,";"} },
    { {"j","=","NUM",";","j","<",NULL,";"} },
    { {"k","=","NUM",";","k","<",NULL,";"} },
    /* if ( ! * ) return -- 空指针/错误检查 */
    { {"if","(","!",NULL,")","return",NULL,NULL} },
    /* * return - NUM ; -- 错误返回 */
    { {NULL,NULL,"return","-","NUM",";",NULL,NULL} },
    /* free ( * ) ; * = -- 资源释放 */
    { {"free","(",NULL,")",";",NULL,"=",NULL} },
    /* * = NULL ; * * -- 置空 */
    { {NULL,"=","NULL",";",NULL,NULL,NULL,NULL} },
};

/* 检查单个 n-gram 是否匹配任一常见固定模式 */
static int analyze_ngram_match_pattern(AnalyzeToken *gram,
                                       const AnalyzeNGramPattern *pat) {
    for (int i = 0; i < ANALYZE_NGRAM_SIZE; i++) {
        if (pat->tok[i] == NULL) continue;  /* 通配符 */
        if (strcmp(gram[i].text, pat->tok[i]) != 0) return 0;
    }
    return 1;
}

/* 检查 n-gram 是否属于常见固定语句（应从重复率统计中排除） */
static int analyze_ngram_is_common(AnalyzeToken *gram) {
    int np = (int)(sizeof(analyze_common_patterns) / sizeof(analyze_common_patterns[0]));
    for (int p = 0; p < np; p++) {
        if (analyze_ngram_match_pattern(gram, &analyze_common_patterns[p]))
            return 1;
    }
    return 0;
}

/* analyze 辅助: 确保项目已加载 */
static void commands_analyze_ensure_loaded(void) {
    if (g_proj->root->child_count == 0 && utils_file_exists(".cboot")) {
        domain_project_free(g_proj);
        g_proj = domain_project_new("cboot_project");
        g_skip_gen = 1;
        parser_parse_cboot_script(".cboot");
        g_skip_gen = 0;
    }
}

/* analyze 辅助: 统计并打印有效代码行数 */
static void commands_analyze_code_lines(AnalyzeMod *mods, int mod_count) {
    int total_lines = 0;
    printf("--- 1. 有效代码行数 ---\n");
    printf("  %-32s %s\n", "模块", "有效行数");
    printf("  %-32s %s\n", "------------------------------", "--------");
    for (int i = 0; i < mod_count; i++) {
        mods[i].code_lines = analyze_count_code_lines(mods[i].code);
        printf("  %-32s %d\n", mods[i].name, mods[i].code_lines);
        total_lines += mods[i].code_lines;
    }
    printf("  %-32s %s\n", "------------------------------", "--------");
    printf("  %-32s %d\n", "总计", total_lines);
    printf("\n");
}

/* analyze 辅助: 计算并打印圈复杂度 */
static void commands_analyze_cyclomatic(AnalyzeFunc *funcs, int func_count) {
    for (int i = 0; i < func_count; i++)
        funcs[i].complexity = analyze_cyclomatic_complexity(funcs[i].code);

    int total_cc = 0, max_cc = 0;
    char max_cc_name[128] = "", max_cc_mod[128] = "";
    printf("--- 2. 圈复杂度 (Cyclomatic Complexity) ---\n");
    printf("  %-44s %s\n", "函数", "复杂度");
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    for (int i = 0; i < func_count; i++) {
        int cc = funcs[i].complexity;
        char display[300];
        snprintf(display, sizeof(display), "%s [%s]", funcs[i].name, funcs[i].module);
        const char *marker = (cc > 15) ? " *** 高" : (cc > 10) ? " ** 中" : "";
        printf("  %-44s %d%s\n", display, cc, marker);
        total_cc += cc;
        if (cc > max_cc) {
            max_cc = cc;
            strncpy(max_cc_name, funcs[i].name, 127);
            strncpy(max_cc_mod, funcs[i].module, 127);
        }
    }
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    if (func_count > 0) {
        printf("  %-44s %d\n", "总计", total_cc);
        printf("  %-44s %d\n", "平均", total_cc / func_count);
        printf("  %-44s %d (%s [%s])\n", "最高", max_cc, max_cc_name, max_cc_mod);
    } else {
        printf("  (无函数数据)\n");
    }
    printf("\n");
}

/* analyze 辅助: 检查 n-gram 是否在其他函数中重复 */
static int analyze_ngram_is_dup(AnalyzeToken **all_toks, int *tok_counts,
                                 int func_count, int fi, int g) {
    for (int j = 0; j < func_count; j++) {
        if (j == fi) continue;
        int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
        if (gj <= 0) continue;
        for (int g2 = 0; g2 < gj; g2++) {
            if (analyze_ngram_equal(&all_toks[fi][g], &all_toks[j][g2]))
                return 1;
        }
    }
    return 0;
}

/* analyze 辅助: 打印重复代码片段 */
/* 打印一对函数间的第一个重复 n-gram；返回 1 表示已打印。
 * 跳过常见固定语句模式，避免误显示控制流模板为重复。 */
static int analyze_print_first_dup_pair(AnalyzeToken **all_toks, int *tok_counts,
                                        int i, int j, AnalyzeFunc *funcs) {
    int gi = tok_counts[i] - ANALYZE_NGRAM_SIZE + 1;
    int gj = tok_counts[j] - ANALYZE_NGRAM_SIZE + 1;
    if (gi <= 0 || gj <= 0) return 0;
    for (int g = 0; g < gi; g++) {
        if (analyze_ngram_is_common(&all_toks[i][g])) continue;
        for (int g2 = 0; g2 < gj; g2++) {
            if (!analyze_ngram_equal(&all_toks[i][g], &all_toks[j][g2])) continue;
            printf("    %s [%s] <-> %s [%s]: ",
                   funcs[i].name, funcs[i].module,
                   funcs[j].name, funcs[j].module);
            for (int k = 0; k < ANALYZE_NGRAM_SIZE && k < 6; k++)
                printf("%s ", all_toks[i][g + k].text);
            if (ANALYZE_NGRAM_SIZE > 6) printf("...");
            printf("\n");
            return 1;
        }
    }
    return 0;
}

static void analyze_print_dup_fragments(AnalyzeToken **all_toks, int *tok_counts,
                                         AnalyzeFunc *funcs, int func_count) {
    printf("\n  重复代码片段 (最多显示 10 处):\n");
    int shown = 0;
    for (int i = 0; i < func_count && shown < 10; i++) {
        for (int j = i + 1; j < func_count && shown < 10; j++) {
            if (analyze_print_first_dup_pair(all_toks, tok_counts, i, j, funcs))
                shown++;
        }
    }
}

/* analyze 辅助: 计算并打印代码重复率 */
static void commands_analyze_duplication(AnalyzeFunc *funcs, int func_count) {
    printf("--- 3. 代码重复率 (Code Duplication) ---\n");
    AnalyzeToken **all_toks = (AnalyzeToken **)calloc(func_count, sizeof(AnalyzeToken *));
    int *tok_counts = (int *)calloc(func_count, sizeof(int));
    for (int i = 0; i < func_count; i++) {
        all_toks[i] = (AnalyzeToken *)calloc(ANALYZE_MAX_TOKENS, sizeof(AnalyzeToken));
        tok_counts[i] = analyze_tokenize(funcs[i].code, all_toks[i], ANALYZE_MAX_TOKENS);
    }

    int total_ngrams = 0, dup_ngrams = 0, skipped_common = 0;
    for (int i = 0; i < func_count; i++) {
        int gi = tok_counts[i] - ANALYZE_NGRAM_SIZE + 1;
        if (gi <= 0) continue;
        for (int g = 0; g < gi; g++) {
            if (analyze_ngram_is_common(&all_toks[i][g])) {
                skipped_common++;
                continue;  /* 跳过常见固定语句（如 for 循环计数器模板） */
            }
            total_ngrams++;
            if (analyze_ngram_is_dup(all_toks, tok_counts, func_count, i, g))
                dup_ngrams++;
        }
    }

    double dup_rate = (total_ngrams > 0) ? (double)dup_ngrams / total_ngrams * 100.0 : 0.0;
    printf("  n-gram 大小: %d tokens\n", ANALYZE_NGRAM_SIZE);
    printf("  总 n-gram 数: %d (已过滤常见固定语句 %d)\n", total_ngrams, skipped_common);
    printf("  重复 n-gram 数: %d\n", dup_ngrams);
    printf("  代码重复率: %.1f%%\n", dup_rate);

    if (dup_ngrams > 0)
        analyze_print_dup_fragments(all_toks, tok_counts, funcs, func_count);

    for (int i = 0; i < func_count; i++) free(all_toks[i]);
    free(all_toks);
    free(tok_counts);
}

/* 在函数体 code 中查找对 name 的调用：name 后紧跟 '('，且 name 是完整标识符。
 * 跳过字符串、字符、注释、预处理指令。返回 1 表示至少调用一次。
 * 仅负责"是否存在调用"的词法判定，不做命名空间解析（由 analyze_resolve_call 完成）。 */
static int analyze_has_call(const char *code, const char *name) {
    if (!code || !*code || !name || !*name) return 0;
    int nlen = (int)strlen(name);
    const char *p = code;
    while (*p) {
        const char *skipped = analyze_skip_non_code(p);
        if (skipped) { p = skipped; continue; }

        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *id_start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            int id_len = (int)(p - id_start);
            if (id_len == nlen && strncmp(id_start, name, nlen) == 0) {
                const char *q = p;
                while (isspace((unsigned char)*q)) q++;
                if (*q == '(') return 1;
            }
            continue;
        }
        p++;
    }
    return 0;
}

/* 命名空间解析：从 caller_module 视角，callee_name 应指向哪个函数？
 * 解析规则（C 语言近似）：
 *   1. 优先匹配 caller_module 内定义的同名函数（同文件作用域优先）
 *   2. 否则若全局只有一个同名函数定义，匹配之
 *   3. 否则无法精确定位（多个同名函数跨模块），返回 -1
 * 返回函数在 funcs[] 中的索引，-1 表示无法定位。 */
static int analyze_resolve_call(const char *callee_name,
                                const char *caller_module,
                                AnalyzeFunc *funcs, int func_count) {
    int same_mod_idx = -1;
    int global_count = 0;
    int global_idx = -1;
    for (int k = 0; k < func_count; k++) {
        if (strcmp(funcs[k].name, callee_name) != 0) continue;
        if (strcmp(funcs[k].module, caller_module) == 0) same_mod_idx = k;
        global_idx = k;
        global_count++;
    }
    if (same_mod_idx >= 0) return same_mod_idx;   /* 同模块定义优先 */
    if (global_count == 1) return global_idx;     /* 全局唯一兜底 */
    return -1;                                    /* 无法精确定位 */
}

/* helpers for commands_analyze_dependency_complexity:
 * 扫描所有函数，按命名空间解析规则填充 call_count。 */
static void dependency_compute_counts(AnalyzeFunc *funcs, int func_count) {
    for (int i = 0; i < func_count; i++)
        funcs[i].call_count = 0;
    for (int i = 0; i < func_count; i++) {
        for (int j = 0; j < func_count; j++) {
            if (i == j) continue;
            if (!analyze_has_call(funcs[j].code, funcs[i].name)) continue;
            int resolved = analyze_resolve_call(funcs[i].name,
                                                funcs[j].module,
                                                funcs, func_count);
            if (resolved == i)
                funcs[i].call_count++;
        }
    }
}

/* helpers for commands_analyze_dependency_complexity:
 * 统计 DC 值（call_count - 1）并打印整张表。 */
static void dependency_report(AnalyzeFunc *funcs, int func_count) {
    int total_dc = 0, max_dc = 0;
    char max_dc_name[128] = "", max_dc_mod[128] = "";
    printf("--- 4. 依赖图复杂度 (Dependency Complexity) ---\n");
    printf("  说明: 被调用次数 - 1，反映修改函数时波及的调用方数量\n");
    printf("  说明: 调用关系经命名空间解析（同模块优先/全局唯一兜底）精确定位\n");
    printf("  %-44s %s\n", "函数", "复杂度");
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    for (int i = 0; i < func_count; i++) {
        int dc = funcs[i].call_count > 0 ? funcs[i].call_count - 1 : 0;
        char display[300];
        snprintf(display, sizeof(display), "%s [%s]", funcs[i].name, funcs[i].module);
        const char *marker = (dc > 10) ? " *** 高" : (dc > 5) ? " ** 中" : "";
        printf("  %-44s %d%s\n", display, dc, marker);
        total_dc += dc;
        if (dc > max_dc) {
            max_dc = dc;
            strncpy(max_dc_name, funcs[i].name, 127);
            strncpy(max_dc_mod, funcs[i].module, 127);
        }
    }
    printf("  %-44s %s\n", "--------------------------------------------", "--------");
    if (func_count > 0) {
        printf("  %-44s %d\n", "总计", total_dc);
        printf("  %-44s %d\n", "平均", total_dc / func_count);
        if (max_dc > 0)
            printf("  %-44s %d (%s [%s])\n", "最高", max_dc, max_dc_name, max_dc_mod);
        else
            printf("  %-44s %d\n", "最高", 0);
    } else {
        printf("  (无函数数据)\n");
    }
    printf("\n");
}

/* analyze 辅助: 计算并打印依赖图复杂度
 * 定义: 函数被 N 个其他函数调用，复杂度 = max(0, N - 1)。
 * 反映修改一个函数时波及的调用方数量。
 * 调用关系通过命名空间解析精确定位：
 *   对每个调用方 G，先确认其代码中存在 name( 调用，
 *   再按"同模块优先 / 全局唯一兜底"规则解析出真正的被调函数 F。 */
static void commands_analyze_dependency_complexity(AnalyzeFunc *funcs, int func_count) {
    dependency_compute_counts(funcs, func_count);
    dependency_report(funcs, func_count);
}

/* analyze 辅助: 遍历域树收集测试统计 (总函数数/有测试用例函数数/总用例数)
 * 跳过 im 导入的 EXTERNAL 模块——这些是 API 副本而非原始定义，
 * 用户无法为其添加测试用例，计入分母会严重稀释覆盖率。
 * 跳过非 API 函数 (mode != API_MODE_API)——static/internal 函数在 _test.c
 * 中不可见 (内部链接, 无导出符号), 无法添加有效测试用例。 */
static void analyze_test_collect(Domain *d, int *total_funcs,
                                  int *tested_funcs, int *total_cases) {
    if (!d) return;
    if (d->type == DOMAIN_MODULE) {
        ModuleDomain *mod = (ModuleDomain *)d;
        if (mod->mode == MOD_MODE_EXTERNAL) return;
    }
    if (d->type == DOMAIN_FUNCTION) {
        FunctionDomain *fd = (FunctionDomain *)d;
        if (fd->mode == API_MODE_API) {
            (*total_funcs)++;
            if (fd->test_cases) {
                (*tested_funcs)++;
                TestCase *tc = fd->test_cases;
                while (tc) { (*total_cases)++; tc = tc->next; }
            }
        }
    }
    for (int i = 0; i < d->child_count; i++)
        analyze_test_collect(d->children[i], total_funcs, tested_funcs, total_cases);
}

/* analyze 辅助: 对模块编译运行 _test.c, 解析输出获取通过数/总数 */
static void analyze_run_module_test(const char *mod_name, int *passed, int *total) {
    *passed = 0; *total = 0;
    char test_file[MAX_PATH_LEN];
    snprintf(test_file, sizeof(test_file), "%s/%s_test.c", mod_name, mod_name);
    if (!utils_file_exists(test_file)) return;  /* 无测试文件 */

    /* 编译: _test.c + 项目 .c 源文件
     * 排除: _test.c, main.c, build/, test/, 导入目录
     * (commands/parser 引用的 g_proj 等全局变量由 _test.c 中的桩提供) */
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "gcc -o /tmp/cboot_%s_test %s "
        "$(find . -name '*.c' -not -path '*/build/*' -not -path '*/test/*' "
        "-not -path './*/domain/*' -not -path './*/cupdate/*' "
        "-not -name '*_test.c' -not -name 'main.c' 2>/dev/null) -I. 2>/dev/null",
        mod_name, test_file);
    int rc = system(cmd);
    if (rc != 0) {
        printf("  %-20s 编译失败\n", mod_name);
        return;
    }

    /* 运行并解析输出 */
    snprintf(cmd, sizeof(cmd), "/tmp/cboot_%s_test 2>/dev/null", mod_name);
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        printf("  %-20s 运行失败\n", mod_name);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int p, t;
        /* 解析 "=== <module> 测试结果: X/Y passed ===" */
        if (sscanf(line, "=== %*[^=]=== 测试结果: %d/%d passed ===", &p, &t) == 2 ||
            sscanf(line, "=== %*s 测试结果: %d/%d passed", &p, &t) == 2) {
            *passed = p; *total = t;
        }
    }
    pclose(fp);
    printf("  %-20s %d/%d passed\n", mod_name, *passed, *total);

    /* 清理临时可执行文件 */
    snprintf(cmd, sizeof(cmd), "rm -f /tmp/cboot_%s_test", mod_name);
    system(cmd);
}

/* analyze 辅助: 遍历域树, 对每个有测试用例的模块运行 _test.c */
static void analyze_test_run_all(Domain *d, int *passed_sum, int *total_sum) {
    if (!d) return;
    if (d->type == DOMAIN_MODULE && d->name) {
        /* 检查模块是否有测试用例 */
        int has_tests = 0;
        for (int i = 0; i < d->child_count; i++) {
            Domain *child = d->children[i];
            if (child->type == DOMAIN_FUNCTION &&
                ((FunctionDomain *)child)->test_cases) {
                has_tests = 1; break;
            }
        }
        if (has_tests) {
            int p = 0, t = 0;
            analyze_run_module_test(d->name, &p, &t);
            *passed_sum += p; *total_sum += t;
        }
    }
    for (int i = 0; i < d->child_count; i++)
        analyze_test_run_all(d->children[i], passed_sum, total_sum);
}

/* analyze 辅助: 计算并打印测试覆盖指标
 * 覆盖率 = 有测试用例的函数数 / 总函数数
 * 通过率 = 编译运行 _test.c 后通过的用例数 / 总用例数 */
static void commands_analyze_test_metrics(void) {
    printf("--- 5. 测试覆盖指标 (Test Metrics) ---\n");
    printf("  说明: test 指令设置输入-预期输出对, tcode 设置自定义代码用例\n");
    printf("  覆盖率 = 有测试用例的函数数 / 总函数数\n");
    printf("  通过率 = 编译运行 _test.c 后通过的用例数 / 总用例数\n\n");

    int total_funcs = 0, tested_funcs = 0, total_cases = 0;
    analyze_test_collect(g_proj->root, &total_funcs, &tested_funcs, &total_cases);

    printf("  统计: 总函数 %d, 有用例函数 %d, 总用例数 %d\n",
           total_funcs, tested_funcs, total_cases);
    if (total_funcs > 0) {
        printf("  测试覆盖率: %.1f%%\n",
               (double)tested_funcs / total_funcs * 100.0);
    }
    if (tested_funcs > 0) {
        printf("\n  编译运行各模块 _test.c:\n");
        int passed_sum = 0, total_run = 0;
        analyze_test_run_all(g_proj->root, &passed_sum, &total_run);
        printf("\n  合计: %d/%d 用例通过\n", passed_sum, total_run);
        if (total_run > 0) {
            printf("  测试通过率: %.1f%%\n",
                   (double)passed_sum / total_run * 100.0);
        } else {
            printf("  测试通过率: N/A (无测试用例运行)\n");
        }
    } else {
        printf("  (尚未为任何函数设置测试用例, 使用 test/tcode 指令添加)\n");
    }
    printf("\n");
}

int commands_cmd_analyze_impl(void)
{
    if (!g_proj || !g_proj->root) {
        printf("错误: 无活动项目\n");
        return -1;
    }

    commands_analyze_ensure_loaded();

    printf("=== CBoot 代码分析报告 ===\n\n");

    /* 1. 有效代码行数 */
    AnalyzeMod mods[ANALYZE_MAX_MODS];
    int mod_count = 0;
    analyze_collect_modules(g_proj->root, mods, &mod_count, ANALYZE_MAX_MODS);
    commands_analyze_code_lines(mods, mod_count);

    /* 2. 圈复杂度 */
    AnalyzeFunc funcs[ANALYZE_MAX_FUNCS];
    int func_count = 0;
    for (int i = 0; i < mod_count && func_count < ANALYZE_MAX_FUNCS; i++)
        analyze_extract_functions(mods[i].name, mods[i].code, funcs, &func_count, ANALYZE_MAX_FUNCS);
    commands_analyze_cyclomatic(funcs, func_count);

    /* 3. 代码重复率 */
    commands_analyze_duplication(funcs, func_count);

    /* 4. 依赖图复杂度 */
    commands_analyze_dependency_complexity(funcs, func_count);

    /* 5. 测试覆盖指标 (函数可选 test 目标) */
    commands_analyze_test_metrics();

    printf("\n=== 分析完成 ===\n");

    for (int i = 0; i < func_count; i++) free(funcs[i].code);
    return 0;
}
