/*
 * test_cupdate_parser.c - cupdate/cupdate_parser.c 单元测试
 * 通过 cup_parse 解析各种 C 声明，验证提取结果
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "cupdate/cupdate.c"
#include "cupdate/cupdate_lexer.c"
#include "cupdate/cupdate_parser.c"

/* mock generator 依赖 */
int generator_generate_cboot_only(Project *proj) { (void)proj; return 0; }

/* 辅助：解析并返回结果，调用者负责 free */
static CUPResult parse_helper(const char *src) {
    CUPResult r;
    cupdate_result_init(&r);
    cup_parse(&r, src, "test.c");
    return r;
}

/* ------------------------------------------------------------------ */
/* cup_parse 基本函数解析                                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_function_basic) {
    TEST_BEGIN("cup_parse 函数声明");
    CUPResult r = parse_helper("int add(int a, int b);");
    ASSERT_EQ_INT(r.error_count, 0, "不应有错误");
    ASSERT_EQ_INT(r.decl_count, 1, "应有 1 个声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_FUNCTION, "应为函数");
    ASSERT_EQ_STR(r.decls[0].name, "add", "名称应为 add");
    ASSERT_EQ_STR(r.decls[0].return_type, "int", "返回类型 int");
    ASSERT_EQ_INT(r.decls[0].param_count, 2, "应有 2 个参数");
    ASSERT_EQ_STR(r.decls[0].params[0].type, "int", "参数0类型");
    ASSERT_EQ_STR(r.decls[0].params[0].name, "a", "参数0名称");
    ASSERT_EQ_STR(r.decls[0].params[1].name, "b", "参数1名称");
    ASSERT_EQ_INT(r.decls[0].is_function_def, 0, "应为声明非定义");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_def) {
    TEST_BEGIN("cup_parse 函数定义（含函数体）");
    CUPResult r = parse_helper("int foo(void) { return 0; }");
    ASSERT_EQ_INT(r.decl_count, 1, "应有 1 个声明");
    ASSERT_EQ_INT(r.decls[0].is_function_def, 1, "应为定义");
    ASSERT_NOT_NULL(r.decls[0].body, "应有函数体");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_void_param) {
    TEST_BEGIN("cup_parse void 参数");
    CUPResult r = parse_helper("void f(void);");
    ASSERT_EQ_INT(r.decl_count, 1, "1 个声明");
    ASSERT_EQ_INT(r.decls[0].param_count, 0, "void 参数应为 0");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_no_params) {
    TEST_BEGIN("cup_parse 无参数函数");
    CUPResult r = parse_helper("int f();");
    ASSERT_EQ_INT(r.decl_count, 1, "1 个声明");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_pointer_param) {
    TEST_BEGIN("cup_parse 指针参数");
    CUPResult r = parse_helper("void f(int *p, char *s);");
    ASSERT_EQ_INT(r.decls[0].param_count, 2, "2 参数");
    ASSERT_EQ_STR(r.decls[0].params[0].type, "int*", "参数0类型 int*");
    ASSERT_EQ_STR(r.decls[0].params[0].name, "p", "参数0名 p");
    ASSERT_EQ_STR(r.decls[0].params[1].type, "char*", "参数1类型 char*");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_const_param) {
    TEST_BEGIN("cup_parse const 参数");
    CUPResult r = parse_helper("int strlen(const char *s);");
    ASSERT_EQ_STR(r.decls[0].params[0].type, "const char*", "应为 const char*");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_static) {
    TEST_BEGIN("cup_parse static 函数");
    CUPResult r = parse_helper("static int helper(int x);");
    ASSERT_EQ_INT(r.decls[0].is_static, 1, "应为 static");
    ASSERT_EQ_INT(r.decls[0].is_api, 0, "static 非 API");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_unsigned_return) {
    TEST_BEGIN("cup_parse unsigned 返回类型");
    CUPResult r = parse_helper("unsigned int get_count(void);");
    ASSERT_EQ_STR(r.decls[0].return_type, "unsigned int", "返回类型");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 结构体解析                                                          */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_struct_basic) {
    TEST_BEGIN("cup_parse 结构体定义");
    CUPResult r = parse_helper("struct Point { int x; int y; };");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_STRUCT, "应为 STRUCT");
    ASSERT_EQ_STR(r.decls[0].name, "Point", "名称 Point");
    ASSERT_EQ_INT(r.decls[0].member_count, 2, "2 成员");
    ASSERT_EQ_STR(r.decls[0].members[0].type, "int", "成员0类型");
    ASSERT_EQ_STR(r.decls[0].members[0].name, "x", "成员0名");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_struct_with_pointer_member) {
    TEST_BEGIN("cup_parse 结构体含指针成员");
    CUPResult r = parse_helper("struct Node { int val; struct Node *next; };");
    ASSERT_EQ_INT(r.decls[0].member_count, 2, "2 成员");
    ASSERT_EQ_STR(r.decls[0].members[1].type, "struct Node*", "指针成员类型");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_struct_empty) {
    TEST_BEGIN("cup_parse 空结构体");
    CUPResult r = parse_helper("struct Empty { };");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].member_count, 0, "0 成员");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* typedef 解析                                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_typedef_basic) {
    TEST_BEGIN("cup_parse typedef 基本类型");
    CUPResult r = parse_helper("typedef int MyInt;");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_TYPEDEF, "应为 TYPEDEF");
    ASSERT_EQ_STR(r.decls[0].name, "MyInt", "名称 MyInt");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_typedef_pointer) {
    TEST_BEGIN("cup_parse typedef 指针");
    CUPResult r = parse_helper("typedef char* String;");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_TYPEDEF, "TYPEDEF");
    ASSERT_EQ_STR(r.decls[0].name, "String", "名称 String");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_typedef_struct) {
    TEST_BEGIN("cup_parse typedef struct");
    CUPResult r = parse_helper("typedef struct { int x; } MyType;");
    /* 解析为 2 个声明：匿名 struct + typedef MyType */
    ASSERT_EQ_INT(r.decl_count, 2, "应为 2 声明（struct + typedef）");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_STRUCT, "0=STRUCT");
    ASSERT_EQ_INT(r.decls[1].kind, CUP_DECL_TYPEDEF, "1=TYPEDEF");
    ASSERT_EQ_STR(r.decls[1].name, "MyType", "typedef 名称 MyType");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 宏定义                                                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_macro_basic) {
    TEST_BEGIN("cup_parse #define 宏");
    CUPResult r = parse_helper("#define MAX_SIZE 100\n");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_MACRO, "应为 MACRO");
    ASSERT_EQ_STR(r.decls[0].name, "MAX_SIZE", "名称 MAX_SIZE");
    ASSERT_EQ_STR(r.decls[0].value, "100", "值 100");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_macro_no_value) {
    TEST_BEGIN("cup_parse #define 无值宏");
    CUPResult r = parse_helper("#define DEBUG\n");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_STR(r.decls[0].name, "DEBUG", "名称 DEBUG");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_macro_function_like) {
    TEST_BEGIN("cup_parse #define 函数式宏");
    CUPResult r = parse_helper("#define ADD(a,b) ((a)+(b))\n");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_STR(r.decls[0].name, "ADD", "名称 ADD");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* #include 指令                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_include_system) {
    TEST_BEGIN("cup_parse #include 系统头文件");
    CUPResult r = parse_helper("#include <stdio.h>\n");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_INCLUDE, "应为 INCLUDE");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_include_local) {
    TEST_BEGIN("cup_parse #include 本地头文件");
    CUPResult r = parse_helper("#include \"myheader.h\"\n");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_INCLUDE, "INCLUDE");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 全局变量                                                            */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_variable_basic) {
    TEST_BEGIN("cup_parse 全局变量");
    CUPResult r = parse_helper("int g_count;");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_VARIABLE, "应为 VARIABLE");
    ASSERT_EQ_STR(r.decls[0].name, "g_count", "名称 g_count");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_variable_with_init) {
    TEST_BEGIN("cup_parse 带初始化的变量");
    CUPResult r = parse_helper("int g_count = 0;");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_STR(r.decls[0].name, "g_count", "名称");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_variable_pointer) {
    TEST_BEGIN("cup_parse 指针变量");
    CUPResult r = parse_helper("char *g_name;");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_VARIABLE, "VARIABLE");
    ASSERT_EQ_STR(r.decls[0].name, "g_name", "名称");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 多声明混合                                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_multiple_decls) {
    TEST_BEGIN("cup_parse 多个声明");
    const char *src =
        "#include <stdio.h>\n"
        "#define MAX 100\n"
        "typedef int MyInt;\n"
        "struct Point { int x; int y; };\n"
        "int add(int a, int b) { return a + b; }\n"
        "int g_count = 0;\n";
    CUPResult r = parse_helper(src);
    ASSERT_EQ_INT(r.decl_count, 6, "应有 6 个声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_INCLUDE, "0=INCLUDE");
    ASSERT_EQ_INT(r.decls[1].kind, CUP_DECL_MACRO, "1=MACRO");
    ASSERT_EQ_INT(r.decls[2].kind, CUP_DECL_TYPEDEF, "2=TYPEDEF");
    ASSERT_EQ_INT(r.decls[3].kind, CUP_DECL_STRUCT, "3=STRUCT");
    ASSERT_EQ_INT(r.decls[4].kind, CUP_DECL_FUNCTION, "4=FUNCTION");
    ASSERT_EQ_INT(r.decls[5].kind, CUP_DECL_VARIABLE, "5=VARIABLE");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 注释处理                                                            */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_with_comments) {
    TEST_BEGIN("cup_parse 带注释");
    const char *src =
        "/* 块注释 */\n"
        "// 行注释\n"
        "int f(void); /* 尾注释 */\n";
    CUPResult r = parse_helper(src);
    ASSERT_EQ_INT(r.error_count, 0, "不应有错误");
    ASSERT_EQ_INT(r.decl_count, 1, "应解析出 1 个声明");
    ASSERT_EQ_STR(r.decls[0].name, "f", "名称 f");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 错误处理                                                            */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_null_input) {
    TEST_BEGIN("cup_parse NULL 输入");
    CUPResult r;
    cupdate_result_init(&r);
    int rc = cup_parse(&r, NULL, "test.c");
    ASSERT_EQ_INT(rc, -1, "NULL source 应返回 -1");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_null_result) {
    TEST_BEGIN("cup_parse NULL result");
    int rc = cup_parse(NULL, "int x;", "test.c");
    ASSERT_EQ_INT(rc, -1, "NULL result 应返回 -1");
    TEST_END();
}

TEST_SUITE(test_parse_empty_source) {
    TEST_BEGIN("cup_parse 空源码");
    CUPResult r = parse_helper("");
    ASSERT_EQ_INT(r.decl_count, 0, "空源码应无声明");
    ASSERT_EQ_INT(r.error_count, 0, "不应有错误");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_only_comments) {
    TEST_BEGIN("cup_parse 仅注释");
    CUPResult r = parse_helper("// only comment\n/* block */\n");
    ASSERT_EQ_INT(r.decl_count, 0, "应无声明");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* enum 解析                                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_enum_basic) {
    TEST_BEGIN("cup_parse enum 定义");
    CUPResult r = parse_helper("enum Color { RED, GREEN, BLUE };");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_STR(r.decls[0].name, "Color", "名称 Color");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_is_calling_convention (间接测试)                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_calling_convention) {
    TEST_BEGIN("cup_parse 调用约定");
    CUPResult r = parse_helper("int __stdcall foo(int x);");
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_STR(r.decls[0].name, "foo", "名称 foo");
    /* call 字段可能存储调用约定 */
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 复杂函数体                                                          */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_complex_function_body) {
    TEST_BEGIN("cup_parse 复杂函数体");
    const char *src =
        "int complex_func(int n) {\n"
        "    int sum = 0;\n"
        "    for (int i = 0; i < n; i++) {\n"
        "        sum += i;\n"
        "    }\n"
        "    return sum;\n"
        "}\n";
    CUPResult r = parse_helper(src);
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_EQ_INT(r.decls[0].is_function_def, 1, "应为定义");
    ASSERT_NOT_NULL(r.decls[0].body, "应有函数体");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_function_with_braces_in_string) {
    TEST_BEGIN("cup_parse 函数体含字符串中的大括号");
    const char *src = "void f(void) { char *s = \"{}\"; }";
    CUPResult r = parse_helper(src);
    ASSERT_EQ_INT(r.decl_count, 1, "1 声明");
    ASSERT_NOT_NULL(r.decls[0].body, "应有函数体");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== cupdate/cupdate_parser.c 测试 ==========\n");

    RUN_SUITE(test_parse_function_basic);
    RUN_SUITE(test_parse_function_def);
    RUN_SUITE(test_parse_function_void_param);
    RUN_SUITE(test_parse_function_no_params);
    RUN_SUITE(test_parse_function_pointer_param);
    RUN_SUITE(test_parse_function_const_param);
    RUN_SUITE(test_parse_function_static);
    RUN_SUITE(test_parse_function_unsigned_return);
    RUN_SUITE(test_parse_struct_basic);
    RUN_SUITE(test_parse_struct_with_pointer_member);
    RUN_SUITE(test_parse_struct_empty);
    RUN_SUITE(test_parse_typedef_basic);
    RUN_SUITE(test_parse_typedef_pointer);
    RUN_SUITE(test_parse_typedef_struct);
    RUN_SUITE(test_parse_macro_basic);
    RUN_SUITE(test_parse_macro_no_value);
    RUN_SUITE(test_parse_macro_function_like);
    RUN_SUITE(test_parse_include_system);
    RUN_SUITE(test_parse_include_local);
    RUN_SUITE(test_parse_variable_basic);
    RUN_SUITE(test_parse_variable_with_init);
    RUN_SUITE(test_parse_variable_pointer);
    RUN_SUITE(test_parse_multiple_decls);
    RUN_SUITE(test_parse_with_comments);
    RUN_SUITE(test_parse_null_input);
    RUN_SUITE(test_parse_null_result);
    RUN_SUITE(test_parse_empty_source);
    RUN_SUITE(test_parse_only_comments);
    RUN_SUITE(test_parse_enum_basic);
    RUN_SUITE(test_parse_calling_convention);
    RUN_SUITE(test_parse_complex_function_body);
    RUN_SUITE(test_parse_function_with_braces_in_string);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
