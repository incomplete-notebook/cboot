/*
 * test_cupdate_lexer.c - cupdate/cupdate_lexer.c 单元测试
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
/* cupdate.c 提供 cupdate_result_init/free 等 */
#include "cupdate/cupdate.c"
#include "cupdate/cupdate_lexer.c"

/* mock：避免引入 parser.c 和 generator.c 的复杂依赖 */
int cup_parse(CUPResult *r, const char *source, const char *filename) {
    (void)r; (void)source; (void)filename;
    return 0;
}
int generator_generate_cboot_only(Project *proj) {
    (void)proj;
    return 0;
}

/* ------------------------------------------------------------------ */
/* cu_lookup_keyword / cu_tok_keyword_name                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lookup_keyword_basic) {
    TEST_BEGIN("cu_lookup_keyword 基本关键字");
    ASSERT_EQ_INT(cu_lookup_keyword("int", 3), CUP_TOK_INT, "int 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("char", 4), CUP_TOK_CHAR_KW, "char 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("void", 4), CUP_TOK_VOID, "void 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("struct", 6), CUP_TOK_STRUCT, "struct 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("return", 6), CUP_TOK_RETURN, "return 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("if", 2), CUP_TOK_IF, "if 应匹配");
    ASSERT_EQ_INT(cu_lookup_keyword("while", 5), CUP_TOK_WHILE, "while 应匹配");
    TEST_END();
}

TEST_SUITE(test_lookup_keyword_not_found) {
    TEST_BEGIN("cu_lookup_keyword 非关键字返回 0");
    ASSERT_EQ_INT(cu_lookup_keyword("foo", 3), 0, "foo 应返回 0");
    ASSERT_EQ_INT(cu_lookup_keyword("intx", 4), 0, "intx 应返回 0");
    ASSERT_EQ_INT(cu_lookup_keyword("", 0), 0, "空串应返回 0");
    TEST_END();
}

TEST_SUITE(test_lookup_keyword_wrong_len) {
    TEST_BEGIN("cu_lookup_keyword 长度不匹配返回 0");
    /* int 长度 3，传 4 应不匹配 */
    ASSERT_EQ_INT(cu_lookup_keyword("int", 4), 0, "错误长度应返回 0");
    TEST_END();
}

TEST_SUITE(test_tok_keyword_name) {
    TEST_BEGIN("cu_tok_keyword_name kind 转名称");
    ASSERT_EQ_STR(cu_tok_keyword_name(CUP_TOK_INT), "int", "INT → int");
    ASSERT_EQ_STR(cu_tok_keyword_name(CUP_TOK_STRUCT), "struct", "STRUCT → struct");
    ASSERT_EQ_STR(cu_tok_keyword_name(CUP_TOK_RETURN), "return", "RETURN → return");
    TEST_END();
}

TEST_SUITE(test_tok_keyword_name_invalid) {
    TEST_BEGIN("cu_tok_keyword_name 无效 kind 返回 NULL");
    ASSERT_NULL(cu_tok_keyword_name(CUP_TOK_ID), "ID 应返回 NULL");
    ASSERT_NULL(cu_tok_keyword_name(CUP_TOK_NUM), "NUM 应返回 NULL");
    ASSERT_NULL(cu_tok_keyword_name(999), "无效 kind 返回 NULL");
    ASSERT_NULL(cu_tok_keyword_name(';'), "单字符返回 NULL");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cu_tok_is_type_kw / is_storage_kw / is_qualifier_kw                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_is_type_kw) {
    TEST_BEGIN("cu_tok_is_type_kw 类型关键字");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_INT), "INT 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_CHAR_KW), "CHAR 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_VOID), "VOID 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_STRUCT), "STRUCT 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_UNION), "UNION 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_ENUM), "ENUM 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_UNSIGNED), "UNSIGNED 是类型");
    ASSERT_TRUE(cu_tok_is_type_kw(CUP_TOK_LONG), "LONG 是类型");
    TEST_END();
}

TEST_SUITE(test_is_type_kw_false) {
    TEST_BEGIN("cu_tok_is_type_kw 非类型关键字");
    ASSERT_FALSE(cu_tok_is_type_kw(CUP_TOK_IF), "IF 不是类型");
    ASSERT_FALSE(cu_tok_is_type_kw(CUP_TOK_RETURN), "RETURN 不是类型");
    ASSERT_FALSE(cu_tok_is_type_kw(CUP_TOK_ID), "ID 不是类型");
    ASSERT_FALSE(cu_tok_is_type_kw(';'), "; 不是类型");
    TEST_END();
}

TEST_SUITE(test_is_storage_kw) {
    TEST_BEGIN("cu_tok_is_storage_kw 存储类");
    ASSERT_TRUE(cu_tok_is_storage_kw(CUP_TOK_STATIC), "STATIC 是存储类");
    ASSERT_TRUE(cu_tok_is_storage_kw(CUP_TOK_EXTERN), "EXTERN 是存储类");
    ASSERT_TRUE(cu_tok_is_storage_kw(CUP_TOK_TYPEDEF), "TYPEDEF 是存储类");
    ASSERT_TRUE(cu_tok_is_storage_kw(CUP_TOK_INLINE), "INLINE 是存储类");
    ASSERT_FALSE(cu_tok_is_storage_kw(CUP_TOK_INT), "INT 不是存储类");
    ASSERT_FALSE(cu_tok_is_storage_kw(CUP_TOK_IF), "IF 不是存储类");
    TEST_END();
}

TEST_SUITE(test_is_qualifier_kw) {
    TEST_BEGIN("cu_tok_is_qualifier_kw 限定符");
    ASSERT_TRUE(cu_tok_is_qualifier_kw(CUP_TOK_CONST), "CONST 是限定符");
    ASSERT_TRUE(cu_tok_is_qualifier_kw(CUP_TOK_VOLATILE), "VOLATILE 是限定符");
    ASSERT_TRUE(cu_tok_is_qualifier_kw(CUP_TOK_RESTRICT), "RESTRICT 是限定符");
    ASSERT_FALSE(cu_tok_is_qualifier_kw(CUP_TOK_INT), "INT 不是限定符");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cu_token_init / free / copy                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_token_init) {
    TEST_BEGIN("cu_token_init 初始化");
    CuToken t;
    cu_token_init(&t);
    ASSERT_EQ_INT(t.kind, 0, "kind 应为 0");
    ASSERT_NULL(t.str, "str 应为 NULL");
    ASSERT_EQ_INT(t.ival, 0, "ival 应为 0");
    ASSERT_EQ_INT(t.line, 0, "line 应为 0");
    TEST_END();
}

TEST_SUITE(test_token_free) {
    TEST_BEGIN("cu_token_free 释放");
    CuToken t;
    cu_token_init(&t);
    t.str = strdup("hello");
    cu_token_free(&t);
    ASSERT_NULL(t.str, "str 应为 NULL");
    TEST_END();
}

TEST_SUITE(test_token_free_null_str) {
    TEST_BEGIN("cu_token_free NULL str 安全");
    CuToken t;
    cu_token_init(&t);
    cu_token_free(&t);  /* str 已为 NULL，不应崩溃 */
    ASSERT_TRUE(1, "安全");
    TEST_END();
}

TEST_SUITE(test_token_copy) {
    TEST_BEGIN("cu_token_copy 深拷贝");
    CuToken src, dst;
    cu_token_init(&src);
    cu_token_init(&dst);
    src.kind = CUP_TOK_ID;
    src.str = strdup("myvar");
    src.line = 10;
    cu_token_copy(&dst, &src);
    ASSERT_EQ_INT(dst.kind, CUP_TOK_ID, "kind 应复制");
    ASSERT_EQ_STR(dst.str, "myvar", "str 应复制");
    ASSERT_TRUE(dst.str != src.str, "应为深拷贝");
    ASSERT_EQ_INT(dst.line, 10, "line 应复制");
    cu_token_free(&src);
    cu_token_free(&dst);
    TEST_END();
}

TEST_SUITE(test_token_copy_null_str) {
    TEST_BEGIN("cu_token_copy 源 str 为 NULL");
    CuToken src, dst;
    cu_token_init(&src);
    cu_token_init(&dst);
    src.kind = ';';
    /* src.str 为 NULL */
    cu_token_copy(&dst, &src);
    ASSERT_EQ_INT(dst.kind, ';', "kind 应复制");
    ASSERT_NULL(dst.str, "str 应为 NULL");
    cu_token_free(&src);
    cu_token_free(&dst);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cu_peek_char / cu_read_char                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_peek_char) {
    TEST_BEGIN("cu_peek_char 前看字符");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "abc", "test", &r);
    /* init 后 cur 已是第一个 token（标识符 abc），pos 已推进到末尾。
     * 重新设置 pos 测试 peek_char 本身 */
    lex.pos = 0;
    ASSERT_EQ_INT(cu_peek_char(&lex, 0), 'a', "offset 0 应为 a");
    ASSERT_EQ_INT(cu_peek_char(&lex, 1), 'b', "offset 1 应为 b");
    ASSERT_EQ_INT(cu_peek_char(&lex, 2), 'c', "offset 2 应为 c");
    ASSERT_EQ_INT(cu_peek_char(&lex, 3), '\0', "越界应返回 0");
    ASSERT_EQ_INT(cu_peek_char(&lex, 100), '\0', "远越界返回 0");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_read_char) {
    TEST_BEGIN("cu_read_char 读取并推进");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "ab", "test", &r);
    /* 注意：lex_init 已预扫描第一个 token 到 cur，pos 已推进 */
    /* 重新初始化测试底层 read_char */
    lex.pos = 0;
    ASSERT_EQ_INT(cu_read_char(&lex), 'a', "应读 a");
    ASSERT_EQ_INT(cu_read_char(&lex), 'b', "应读 b");
    ASSERT_EQ_INT(cu_read_char(&lex), '\0', "应读 EOF");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_read_char_newline) {
    TEST_BEGIN("cu_read_char 换行重置行列");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "a\nb", "test", &r);
    lex.pos = 0;
    lex.line = 1;
    lex.col = 1;
    cu_read_char(&lex);  /* a */
    ASSERT_EQ_INT(lex.col, 2, "读 a 后 col=2");
    cu_read_char(&lex);  /* \n */
    ASSERT_EQ_INT(lex.line, 2, "读 \\n 后 line=2");
    ASSERT_EQ_INT(lex.col, 1, "读 \\n 后 col=1");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cu_lex_init / cu_lex_cur / cu_lex_next / cu_lex_peek                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_init_and_cur) {
    TEST_BEGIN("cu_lex_init 预扫描第一个 token");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "第一个 token 应为 INT");
    ASSERT_EQ_STR(lex.cur.str, "int", "str 应为 int");
    ASSERT_EQ_INT(lex.cur.line, 1, "行号应为 1");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_next) {
    TEST_BEGIN("cu_lex_next 推进 token");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "cur=int");
    ASSERT_EQ_INT(cu_lex_next(&lex), CUP_TOK_ID, "next 后 cur=id");
    ASSERT_EQ_STR(lex.cur.str, "x", "str 应为 x");
    ASSERT_EQ_INT(cu_lex_next(&lex), ';', "next 后 cur=;");
    ASSERT_EQ_INT(cu_lex_next(&lex), CUP_TOK_EOF, "next 后 cur=EOF");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_peek) {
    TEST_BEGIN("cu_lex_peek 前看下一 token");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "cur=int");
    ASSERT_EQ_INT(cu_lex_peek(&lex), CUP_TOK_ID, "peek 应为 ID");
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "peek 后 cur 仍为 int");
    ASSERT_EQ_INT(cu_lex_next(&lex), CUP_TOK_ID, "next 后 cur=id");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_peek_eof) {
    TEST_BEGIN("cu_lex_peek 到达 EOF");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "x", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "cur=id");
    ASSERT_EQ_INT(cu_lex_peek(&lex), CUP_TOK_EOF, "peek 应为 EOF");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_empty_input) {
    TEST_BEGIN("cu_lex 空输入");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_EOF, "空输入 cur 应为 EOF");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 标识符与关键字                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_identifier) {
    TEST_BEGIN("cu_lex 标识符");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "foo bar123 _baz", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "foo 应为 ID");
    ASSERT_EQ_STR(lex.cur.str, "foo", "str=foo");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "bar123 应为 ID");
    ASSERT_EQ_STR(lex.cur.str, "bar123", "str=bar123");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "_baz 应为 ID");
    ASSERT_EQ_STR(lex.cur.str, "_baz", "str=_baz");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_keywords) {
    TEST_BEGIN("cu_lex 关键字识别");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int char void struct return if while for", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "int");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_CHAR_KW, "char");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_VOID, "void");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_STRUCT, "struct");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_RETURN, "return");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_IF, "if");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_WHILE, "while");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_FOR, "for");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 数字字面量                                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_numbers) {
    TEST_BEGIN("cu_lex 数字字面量");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "42 0x1F 3.14 100L", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_NUM, "42 应为 NUM");
    ASSERT_EQ_INT(lex.cur.ival, 42, "ival=42");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_NUM, "0x1F 应为 NUM");
    ASSERT_EQ_INT(lex.cur.ival, 31, "0x1F=31");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_NUM, "3.14 应为 NUM");
    ASSERT_TRUE(lex.cur.is_float, "3.14 应为浮点");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_NUM, "100L 应为 NUM");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 字符串与字符字面量                                                    */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_string) {
    TEST_BEGIN("cu_lex 字符串字面量");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "\"hello world\"", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_STR, "应为 STR");
    ASSERT_NOT_NULL(lex.cur.str, "str 不应为 NULL");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_string_with_escapes) {
    TEST_BEGIN("cu_lex 字符串含转义");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "\"line1\\nline2\\ttab\"", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_STR, "应为 STR");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_char_literal) {
    TEST_BEGIN("cu_lex 字符字面量");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "'a' '\\n' '\\0'", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_CHAR, "'a' 应为 CHAR");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_CHAR, "'\\n' 应为 CHAR");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_CHAR, "'\\0' 应为 CHAR");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 运算符                                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_operators) {
    TEST_BEGIN("cu_lex 运算符");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "== != <= >= && || ++ -- -> << >>", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_EQ, "==");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_NEQ, "!=");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_LE, "<=");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_GE, ">=");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_AND_AND, "&&");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_OR_OR, "||");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INC, "++");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_DEC, "--");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_PTR, "->");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_SHL, "<<");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_SHR, ">>");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_single_chars) {
    TEST_BEGIN("cu_lex 单字符 token");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "; , ( ) { } [ ] + - * / % = < > & | ^ ~ ! ?", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), ';', ";");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), ',', ",");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '(', "(");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), ')', ")");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '{', "{");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '}', "}");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 注释与空白                                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_skip_line_comment) {
    TEST_BEGIN("cu_lex 跳过行注释");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "// 这是注释\nint x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "应跳过注释到 int");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_skip_block_comment) {
    TEST_BEGIN("cu_lex 跳过块注释");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "/* 块注释 */ int x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "应跳过块注释到 int");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_skip_multiline_block_comment) {
    TEST_BEGIN("cu_lex 跳过多行块注释");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "/* 第一行\n第二行\n第三行 */ x", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "应跳过多行块注释到 ID");
    ASSERT_EQ_STR(lex.cur.str, "x", "str=x");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_skip_whitespace) {
    TEST_BEGIN("cu_lex 跳过空白");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "   \t\n  int", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "应跳过空白到 int");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 预处理器指令                                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_preprocessor) {
    TEST_BEGIN("cu_lex 预处理器指令");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "#include <stdio.h>\nint x;", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_PP, "应为 PP");
    /* PP 后是 < stdio . h > 等token */
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '<', "之后应为 <");
    /* 跳过 stdio.h> 到换行后的 int */
    while (cu_lex_cur(&lex) != CUP_TOK_INT && cu_lex_cur(&lex) != CUP_TOK_EOF) {
        cu_lex_next(&lex);
    }
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "最终应到 int");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_preprocessor_not_at_line_start) {
    TEST_BEGIN("cu_lex 非行首 # 不是 PP");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "x # y", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "cur=id");
    cu_lex_next(&lex);
    /* 非行首 # 应为单字符 # */
    ASSERT_EQ_INT(cu_lex_cur(&lex), '#', "非行首 # 应为单字符");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 错误处理                                                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_error_recording) {
    TEST_BEGIN("cu_lex_error 记录错误");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "x", "test.c", &r);
    cu_lex_error(&lex, "测试错误消息");
    ASSERT_TRUE(r.error_count > 0, "应有错误记录");
    cupdate_result_free(&r);
    /* cu_lex_free 不能调用，因为 r 已 free */
    cu_token_free(&lex.cur);
    cu_token_free(&lex.peek);
    TEST_END();
}

TEST_SUITE(test_lex_error_null_filename) {
    TEST_BEGIN("cu_lex_error NULL filename 显示 <input>");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "x", NULL, &r);
    cu_lex_error(&lex, "msg");
    ASSERT_TRUE(r.error_count > 0, "应有错误记录");
    cupdate_result_free(&r);
    cu_token_free(&lex.cur);
    cu_token_free(&lex.peek);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 完整代码片段                                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_lex_c_function) {
    TEST_BEGIN("cu_lex 完整 C 函数");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int add(int a, int b) { return a + b; }", "test", &r);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "int");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "add");
    ASSERT_EQ_STR(lex.cur.str, "add", "add");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '(', "(");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "int");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "a");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), ',', ",");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_INT, "int");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_ID, "b");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), ')', ")");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), '{', "{");
    cu_lex_next(&lex);
    ASSERT_EQ_INT(cu_lex_cur(&lex), CUP_TOK_RETURN, "return");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_lex_line_tracking) {
    TEST_BEGIN("cu_lex 行号追踪");
    CuLexer lex;
    CUPResult r;
    cupdate_result_init(&r);
    cu_lex_init(&lex, "int a;\nint b;\nint c;", "test", &r);
    ASSERT_EQ_INT(lex.cur.line, 1, "a 在第 1 行");
    cu_lex_next(&lex); cu_lex_next(&lex); cu_lex_next(&lex); /* a ; */
    cu_lex_next(&lex);  /* int on line 2 */
    ASSERT_EQ_INT(lex.cur.line, 2, "b 的 int 在第 2 行");
    cu_lex_free(&lex);
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== cupdate/cupdate_lexer.c 测试 ==========\n");

    RUN_SUITE(test_lookup_keyword_basic);
    RUN_SUITE(test_lookup_keyword_not_found);
    RUN_SUITE(test_lookup_keyword_wrong_len);
    RUN_SUITE(test_tok_keyword_name);
    RUN_SUITE(test_tok_keyword_name_invalid);
    RUN_SUITE(test_is_type_kw);
    RUN_SUITE(test_is_type_kw_false);
    RUN_SUITE(test_is_storage_kw);
    RUN_SUITE(test_is_qualifier_kw);
    RUN_SUITE(test_token_init);
    RUN_SUITE(test_token_free);
    RUN_SUITE(test_token_free_null_str);
    RUN_SUITE(test_token_copy);
    RUN_SUITE(test_token_copy_null_str);
    RUN_SUITE(test_peek_char);
    RUN_SUITE(test_read_char);
    RUN_SUITE(test_read_char_newline);
    RUN_SUITE(test_lex_init_and_cur);
    RUN_SUITE(test_lex_next);
    RUN_SUITE(test_lex_peek);
    RUN_SUITE(test_lex_peek_eof);
    RUN_SUITE(test_lex_empty_input);
    RUN_SUITE(test_lex_identifier);
    RUN_SUITE(test_lex_keywords);
    RUN_SUITE(test_lex_numbers);
    RUN_SUITE(test_lex_string);
    RUN_SUITE(test_lex_string_with_escapes);
    RUN_SUITE(test_lex_char_literal);
    RUN_SUITE(test_lex_operators);
    RUN_SUITE(test_lex_single_chars);
    RUN_SUITE(test_lex_skip_line_comment);
    RUN_SUITE(test_lex_skip_block_comment);
    RUN_SUITE(test_lex_skip_multiline_block_comment);
    RUN_SUITE(test_lex_skip_whitespace);
    RUN_SUITE(test_lex_preprocessor);
    RUN_SUITE(test_lex_preprocessor_not_at_line_start);
    RUN_SUITE(test_lex_error_recording);
    RUN_SUITE(test_lex_error_null_filename);
    RUN_SUITE(test_lex_c_function);
    RUN_SUITE(test_lex_line_tracking);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
