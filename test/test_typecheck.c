/*
 * test_typecheck.c - typecheck/typecheck.c 单元测试
 * 覆盖类型检查、内置类型识别、typedef 解析、值校验等所有逻辑
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"

/* ------------------------------------------------------------------ */
/* typecheck_type_checker_is_builtin                                   */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_is_builtin_basic) {
    TEST_BEGIN("is_builtin 基本类型");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("void"), "void");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("char"), "char");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("short"), "short");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("int"), "int");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("long"), "long");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("float"), "float");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("double"), "double");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("signed"), "signed");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("unsigned"), "unsigned");
    TEST_END();
}

TEST_SUITE(test_is_builtin_stdint) {
    TEST_BEGIN("is_builtin stdint 类型");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("size_t"), "size_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("ssize_t"), "ssize_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("int8_t"), "int8_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("int16_t"), "int16_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("int32_t"), "int32_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("int64_t"), "int64_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("uint8_t"), "uint8_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("uint16_t"), "uint16_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("uint32_t"), "uint32_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("uint64_t"), "uint64_t");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("bool"), "bool");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("FILE"), "FILE");
    TEST_END();
}

TEST_SUITE(test_is_builtin_compound) {
    TEST_BEGIN("is_builtin 复合类型");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("unsigned char"), "unsigned char");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("unsigned short"), "unsigned short");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("unsigned int"), "unsigned int");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("unsigned long"), "unsigned long");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("signed char"), "signed char");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("signed int"), "signed int");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("long int"), "long int");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("long long"), "long long");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("short int"), "short int");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("long double"), "long double");
    ASSERT_TRUE(typecheck_type_checker_is_builtin("long unsigned int"), "long unsigned int");
    TEST_END();
}

TEST_SUITE(test_is_builtin_not_builtin) {
    TEST_BEGIN("is_builtin 非内置");
    ASSERT_FALSE(typecheck_type_checker_is_builtin("MyStruct"), "自定义结构体");
    ASSERT_FALSE(typecheck_type_checker_is_builtin("foo_t"), "自定义 typedef");
    ASSERT_FALSE(typecheck_type_checker_is_builtin("MyClass"), "类名");
    TEST_END();
}

TEST_SUITE(test_is_builtin_null) {
    TEST_BEGIN("is_builtin NULL/空串");
    ASSERT_FALSE(typecheck_type_checker_is_builtin(NULL), "NULL 应返回 0");
    ASSERT_FALSE(typecheck_type_checker_is_builtin(""), "空串应返回 0");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* typecheck_type_checker_init                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_init_basic) {
    TEST_BEGIN("type_checker_init 基本初始化");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_TRUE(tc.scope == (Domain *)m, "scope 应设置");
    ASSERT_NULL(tc.error_buf, "error_buf 应为 NULL");
    ASSERT_EQ_INT(tc.error_len, 0, "error_len 应为 0");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_init_null) {
    TEST_BEGIN("type_checker_init NULL 安全");
    typecheck_type_checker_init(NULL, NULL);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* typecheck_type_checker_validate                                     */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_validate_builtin) {
    TEST_BEGIN("validate 内置类型应通过");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "int"), 0, "int 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "char*"), 0, "char* 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "void**"), 0, "void** 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "unsigned int"), 0, "unsigned int 应通过");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_with_qualifiers) {
    TEST_BEGIN("validate 带 qualifier");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "const int"), 0, "const int 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "struct Foo*"), -1, "struct Foo 未定义应失败");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "volatile char"), 0, "volatile char 应通过");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_user_struct_in_scope) {
    TEST_BEGIN("validate scope 内用户结构体");
    ModuleDomain *m = domain_module_domain_new("m");
    StructDomain *s = domain_struct_domain_new("MyStruct");
    domain_domain_add_child((Domain *)m, (Domain *)s);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "MyStruct"), 0, "应找到 MyStruct");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "MyStruct*"), 0, "MyStruct* 应通过");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_user_type_in_scope) {
    TEST_BEGIN("validate scope 内用户 typedef");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeDomain *t = domain_type_domain_new("MyType");
    domain_domain_add_child((Domain *)m, (Domain *)t);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "MyType"), 0, "应找到 MyType");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_undefined_type) {
    TEST_BEGIN("validate 未定义类型失败");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "Unknown"), -1, "未定义应返回 -1");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_walks_up_scope) {
    TEST_BEGIN("validate 父 scope 中的类型");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("sub");
    StructDomain *s = domain_struct_domain_new("ParentStruct");
    domain_domain_add_child((Domain *)root, (Domain *)sub);
    domain_domain_add_child((Domain *)root, (Domain *)s);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)sub);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "ParentStruct"), 0, "应在父 scope 找到");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_validate_api_in_submodule) {
    TEST_BEGIN("validate 子模块 API 类型");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("sub");
    StructDomain *s = domain_struct_domain_new("ApiStruct");
    s->mode = API_MODE_API;
    domain_domain_add_child((Domain *)root, (Domain *)sub);
    domain_domain_add_child((Domain *)sub, (Domain *)s);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)root);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "ApiStruct"), 0, "应找到子模块 API");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_validate_array_dims) {
    TEST_BEGIN("validate 数组维度");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "int[10]"), 0, "int[10] 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "char[32]"), 0, "char[32] 应通过");
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, "int[2][3]"), 0, "多维数组应通过");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_validate_null) {
    TEST_BEGIN("validate NULL 安全");
    ASSERT_EQ_INT(typecheck_type_checker_validate(NULL, "int"), -1, "NULL tc 返回 -1");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_INT(typecheck_type_checker_validate(&tc, NULL), -1, "NULL type 返回 -1");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* type_checker_resolve_typedef                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_resolve_typedef_builtin) {
    TEST_BEGIN("resolve_typedef 内置类型原样返回");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_STR(type_checker_resolve_typedef(&tc, "int"), "int", "int 原样返回");
    ASSERT_EQ_STR(type_checker_resolve_typedef(&tc, "char"), "char", "char 原样返回");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_resolve_typedef_rename) {
    TEST_BEGIN("resolve_typedef rename 类型");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeDomain *t = domain_type_domain_new("MyInt");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)t, "int");
    domain_domain_add_child((Domain *)m, (Domain *)t);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_STR(type_checker_resolve_typedef(&tc, "MyInt"), "int", "应解析为 int");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_resolve_typedef_struct_mode) {
    TEST_BEGIN("resolve_typedef struct 模式原名返回");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeDomain *t = domain_type_domain_new("MyType");
    t->mode = TYPE_MODE_STRUCT;  /* 非 rename */
    domain_domain_add_child((Domain *)m, (Domain *)t);
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_STR(type_checker_resolve_typedef(&tc, "MyType"), "MyType", "struct 模式应原样");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_resolve_typedef_undefined) {
    TEST_BEGIN("resolve_typedef 未定义原名返回");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_EQ_STR(type_checker_resolve_typedef(&tc, "Unknown"), "Unknown", "未定义原样返回");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_resolve_typedef_null) {
    TEST_BEGIN("resolve_typedef NULL 安全");
    ASSERT_EQ_STR(type_checker_resolve_typedef(NULL, "int"), "int", "NULL tc 原样返回");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    ASSERT_NULL(type_checker_resolve_typedef(&tc, NULL), "NULL type 返回 NULL");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* typecheck_type_checker_validate_value                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_validate_value_integer) {
    TEST_BEGIN("validate_value 整数");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "42"), 0, "42 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "-1"), 0, "-1 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "+10"), 0, "+10 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "0x1F"), 0, "0x1F 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "0XFF"), 0, "0XFF 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "  123  "), 0, "空白应允许");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "abc"), -1, "abc 应无效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "12.5"), -1, "12.5 应无效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", ""), -1, "空串应无效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", "0x"), -1, "0x 后无数字应无效");
    TEST_END();
}

TEST_SUITE(test_validate_value_integer_types) {
    TEST_BEGIN("validate_value 各种整数类型");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("char", "65"), 0, "char");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("short", "100"), 0, "short");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("long", "1000"), 0, "long");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("signed", "10"), 0, "signed");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("unsigned", "10"), 0, "unsigned");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("size_t", "100"), 0, "size_t");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int32_t", "100"), 0, "int32_t");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("uint8_t", "200"), 0, "uint8_t");
    TEST_END();
}

TEST_SUITE(test_validate_value_float) {
    TEST_BEGIN("validate_value 浮点");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "3.14"), 0, "3.14 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("double", "2.71828"), 0, "double 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "-1.5"), 0, "-1.5 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "+0.5"), 0, "+0.5 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "1e10"), 0, "1e10 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "1.5E-3"), 0, "1.5E-3 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "3.14f"), 0, "3.14f 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "100"), 0, "整数 100 应有效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "."), -1, "'.' 应无效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "abc"), -1, "abc 应无效");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("float", "1e"), -1, "1e 应无效");
    TEST_END();
}

TEST_SUITE(test_validate_value_void_any) {
    TEST_BEGIN("validate_value void/FILE/bool 接受任意值");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("void", "anything"), 0, "void 接受任意");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("FILE", "stdin"), 0, "FILE 接受任意");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("bool", "true"), 0, "bool 接受任意");
    TEST_END();
}

TEST_SUITE(test_validate_value_pointer) {
    TEST_BEGIN("validate_value 指针类型");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int*", "42"), 0, "int* 视为 int");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("char *", "65"), 0, "char * 视为 char");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("void**", "100"), 0, "void** 视为 void");
    TEST_END();
}

TEST_SUITE(test_validate_value_user_type) {
    TEST_BEGIN("validate_value 用户定义类型接受任意");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("MyStruct", "anything"), 0, "用户类型接受任意");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("foo_t", "bar"), 0, "用户类型接受任意");
    TEST_END();
}

TEST_SUITE(test_validate_value_null) {
    TEST_BEGIN("validate_value NULL 安全");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value(NULL, "x"), -1, "NULL type 返回 -1");
    ASSERT_EQ_INT(typecheck_type_checker_validate_value("int", NULL), -1, "NULL value 返回 -1");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 边界：超长类型字符串                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_validate_long_type) {
    TEST_BEGIN("validate 超长类型字符串（截断处理）");
    ModuleDomain *m = domain_module_domain_new("m");
    TypeChecker tc;
    typecheck_type_checker_init(&tc, (Domain *)m);
    /* 构造超长字符串（500 字节），应被截断为 255 字符后处理 */
    char long_type[512];
    long_type[0] = '\0';
    for (int i = 0; i < 50; i++) strcat(long_type, "unsigned ");
    strcat(long_type, "int");
    /* 不应崩溃，应返回 0 或 -1 */
    int rc = typecheck_type_checker_validate(&tc, long_type);
    ASSERT_TRUE(rc == 0 || rc == -1, "不应崩溃，应返回 0 或 -1");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== typecheck/typecheck.c 测试 ==========\n");

    RUN_SUITE(test_is_builtin_basic);
    RUN_SUITE(test_is_builtin_stdint);
    RUN_SUITE(test_is_builtin_compound);
    RUN_SUITE(test_is_builtin_not_builtin);
    RUN_SUITE(test_is_builtin_null);
    RUN_SUITE(test_init_basic);
    RUN_SUITE(test_init_null);
    RUN_SUITE(test_validate_builtin);
    RUN_SUITE(test_validate_with_qualifiers);
    RUN_SUITE(test_validate_user_struct_in_scope);
    RUN_SUITE(test_validate_user_type_in_scope);
    RUN_SUITE(test_validate_undefined_type);
    RUN_SUITE(test_validate_walks_up_scope);
    RUN_SUITE(test_validate_api_in_submodule);
    RUN_SUITE(test_validate_array_dims);
    RUN_SUITE(test_validate_null);
    RUN_SUITE(test_resolve_typedef_builtin);
    RUN_SUITE(test_resolve_typedef_rename);
    RUN_SUITE(test_resolve_typedef_struct_mode);
    RUN_SUITE(test_resolve_typedef_undefined);
    RUN_SUITE(test_resolve_typedef_null);
    RUN_SUITE(test_validate_value_integer);
    RUN_SUITE(test_validate_value_integer_types);
    RUN_SUITE(test_validate_value_float);
    RUN_SUITE(test_validate_value_void_any);
    RUN_SUITE(test_validate_value_pointer);
    RUN_SUITE(test_validate_value_user_type);
    RUN_SUITE(test_validate_value_null);
    RUN_SUITE(test_validate_long_type);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
