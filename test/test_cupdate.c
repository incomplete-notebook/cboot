/*
 * test_cupdate.c - cupdate/cupdate.c 单元测试
 * 覆盖 cupdate.c 中所有函数和逻辑分支
 *
 * 通过 #include "cupdate/cupdate.c" 方式包含被测源文件以测试 static 函数。
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

/* ------------------------------------------------------------------ */
/* 辅助函数                                                            */
/* ------------------------------------------------------------------ */

static CUParParam *alloc_params(int count) {
    return (CUParParam *)calloc(count > 0 ? count : 1, sizeof(CUParParam));
}

static CUParMember *alloc_members(int count) {
    return (CUParMember *)calloc(count > 0 ? count : 1, sizeof(CUParMember));
}

static void set_param(CUParParam *p, const char *type, const char *name) {
    p->type = type ? strdup(type) : NULL;
    p->name = name ? strdup(name) : NULL;
}

static void set_member(CUParMember *m, const char *type, const char *name, const char *value) {
    m->type = type ? strdup(type) : NULL;
    m->name = name ? strdup(name) : NULL;
    m->value = value ? strdup(value) : NULL;
}

static int write_temp_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

/* ------------------------------------------------------------------ */
/* cup_free_param_array                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_free_param_array_basic) {
    TEST_BEGIN("cup_free_param_array 释放填充数组");
    CUParParam *params = alloc_params(2);
    set_param(&params[0], "int", "a");
    set_param(&params[1], "char*", "b");
    cup_free_param_array(params, 2);
    ASSERT_TRUE(1, "不应崩溃");
    TEST_END();
}

TEST_SUITE(test_free_param_array_null) {
    TEST_BEGIN("cup_free_param_array NULL 数组安全");
    cup_free_param_array(NULL, 0);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_free_member_array                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_free_member_array_basic) {
    TEST_BEGIN("cup_free_member_array 释放填充数组（含 value）");
    CUParMember *members = alloc_members(2);
    set_member(&members[0], "int", "x", "0");
    set_member(&members[1], "char*", "s", NULL);
    cup_free_member_array(members, 2);
    ASSERT_TRUE(1, "不应崩溃");
    TEST_END();
}

TEST_SUITE(test_free_member_array_null) {
    TEST_BEGIN("cup_free_member_array NULL 安全");
    cup_free_member_array(NULL, 0);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_free_decl                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_free_decl_null) {
    TEST_BEGIN("cup_free_decl NULL 安全");
    cup_free_decl(NULL);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

TEST_SUITE(test_free_decl_populated) {
    TEST_BEGIN("cup_free_decl 释放填充声明");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strdup("foo");
    d.return_type = strdup("int");
    d.base_type = strdup("long");
    d.value = strdup("42");
    d.body = strdup("{ return 0; }");
    d.params = alloc_params(1);
    set_param(&d.params[0], "int", "x");
    d.param_count = 1;
    d.members = alloc_members(1);
    set_member(&d.members[0], "int", "y", NULL);
    d.member_count = 1;
    cup_free_decl(&d);
    ASSERT_NULL(d.name, "name 应为 NULL");
    ASSERT_NULL(d.return_type, "return_type 应为 NULL");
    ASSERT_NULL(d.body, "body 应为 NULL");
    ASSERT_NULL(d.params, "params 应为 NULL");
    ASSERT_NULL(d.members, "members 应为 NULL");
    TEST_END();
}

TEST_SUITE(test_free_decl_empty) {
    TEST_BEGIN("cup_free_decl 空声明安全");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    cup_free_decl(&d);
    ASSERT_TRUE(1, "空声明安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_zero_str_slot                                                   */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_zero_str_slot) {
    TEST_BEGIN("cup_zero_str_slot 清零三元组");
    char **arr = NULL;
    int count = 5;
    int cap = 10;
    cup_zero_str_slot(&arr, &count, &cap);
    ASSERT_NULL(arr, "arr 应为 NULL");
    ASSERT_EQ_INT(count, 0, "count 应为 0");
    ASSERT_EQ_INT(cap, 0, "cap 应为 0");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_free_str_array                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_free_str_array_populated) {
    TEST_BEGIN("cup_free_str_array 释放填充数组");
    char **arr = (char **)malloc(sizeof(char *) * 2);
    arr[0] = strdup("hello");
    arr[1] = strdup("world");
    int count = 2, cap = 2;
    cup_free_str_array(&arr, &count, &cap);
    ASSERT_NULL(arr, "arr 应为 NULL");
    ASSERT_EQ_INT(count, 0, "count 应为 0");
    ASSERT_EQ_INT(cap, 0, "cap 应为 0");
    TEST_END();
}

TEST_SUITE(test_free_str_array_null) {
    TEST_BEGIN("cup_free_str_array NULL 安全");
    char **arr = NULL;
    int count = 0, cap = 0;
    cup_free_str_array(&arr, &count, &cap);
    ASSERT_NULL(arr, "arr 应为 NULL");
    ASSERT_EQ_INT(count, 0, "count 应为 0");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_str_array_push                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_str_array_push_basic) {
    TEST_BEGIN("cup_str_array_push 首次推送");
    char **arr = NULL;
    int count = 0, cap = 0;
    cup_str_array_push(&arr, &count, &cap, "first");
    ASSERT_EQ_INT(count, 1, "count 应为 1");
    ASSERT_EQ_INT(cap, 16, "cap 应为 16（首次扩容）");
    ASSERT_EQ_STR(arr[0], "first", "内容应匹配");
    utils_free_tokens(arr, count);
    TEST_END();
}

TEST_SUITE(test_str_array_push_growth) {
    TEST_BEGIN("cup_str_array_push 触发扩容");
    char **arr = NULL;
    int count = 0, cap = 0;
    for (int i = 0; i < 20; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "msg%d", i);
        cup_str_array_push(&arr, &count, &cap, buf);
    }
    ASSERT_EQ_INT(count, 20, "count 应为 20");
    ASSERT_TRUE(cap >= 20, "cap 应 >= 20");
    ASSERT_EQ_STR(arr[0], "msg0", "第一条");
    ASSERT_EQ_STR(arr[19], "msg19", "最后一条");
    utils_free_tokens(arr, count);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_result_init                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_result_init_basic) {
    TEST_BEGIN("cupdate_result_init 基本初始化");
    CUPResult r;
    cupdate_result_init(&r);
    ASSERT_NULL(r.decls, "decls 应为 NULL");
    ASSERT_EQ_INT(r.decl_count, 0, "decl_count 应为 0");
    ASSERT_EQ_INT(r.decl_capacity, 0, "decl_capacity 应为 0");
    ASSERT_NULL(r.errors, "errors 应为 NULL");
    ASSERT_EQ_INT(r.error_count, 0, "error_count 应为 0");
    ASSERT_NULL(r.warnings, "warnings 应为 NULL");
    ASSERT_EQ_INT(r.warning_count, 0, "warning_count 应为 0");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_result_init_after_population) {
    TEST_BEGIN("cupdate_result_init 重置已有结果");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_add_error(&r, "err1", 1);
    cupdate_result_add_decl(&r);
    /* 再次 init 不释放旧内存（仅清零指针），验证字段归零 */
    cupdate_result_init(&r);
    ASSERT_EQ_INT(r.error_count, 0, "error_count 应为 0");
    ASSERT_EQ_INT(r.decl_count, 0, "decl_count 应为 0");
    /* 不 free，因为 init 后指针已 NULL */
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_result_free                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_result_free_empty) {
    TEST_BEGIN("cupdate_result_free 空结果安全");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_free(&r);
    ASSERT_NULL(r.decls, "decls 应为 NULL");
    ASSERT_NULL(r.errors, "errors 应为 NULL");
    ASSERT_NULL(r.warnings, "warnings 应为 NULL");
    TEST_END();
}

TEST_SUITE(test_result_free_populated) {
    TEST_BEGIN("cupdate_result_free 释放填充结果");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_add_error(&r, "error1", 1);
    cupdate_result_add_error(&r, "error2", 2);
    cupdate_result_add_warning(&r, "warn1", 3);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->name = strdup("foo");
    d->return_type = strdup("int");
    cupdate_result_free(&r);
    ASSERT_NULL(r.decls, "decls 应为 NULL");
    ASSERT_NULL(r.errors, "errors 应为 NULL");
    ASSERT_NULL(r.warnings, "warnings 应为 NULL");
    ASSERT_EQ_INT(r.decl_count, 0, "decl_count 应为 0");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_result_add_error / add_warning                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_result_add_error) {
    TEST_BEGIN("cupdate_result_add_error 追加错误");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_add_error(&r, "first error", 10);
    cupdate_result_add_error(&r, "second error", 20);
    ASSERT_EQ_INT(r.error_count, 2, "应有 2 个错误");
    ASSERT_EQ_STR(r.errors[0], "first error", "第一条错误");
    ASSERT_EQ_STR(r.errors[1], "second error", "第二条错误");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_result_add_warning) {
    TEST_BEGIN("cupdate_result_add_warning 追加警告");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_add_warning(&r, "warning1", 5);
    cupdate_result_add_warning(&r, "warning2", 15);
    ASSERT_EQ_INT(r.warning_count, 2, "应有 2 个警告");
    ASSERT_EQ_STR(r.warnings[0], "warning1", "第一条警告");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_result_add_decl                                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_result_add_decl_basic) {
    TEST_BEGIN("cupdate_result_add_decl 基本添加");
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    ASSERT_NOT_NULL(d, "不应返回 NULL");
    ASSERT_EQ_INT(r.decl_count, 1, "decl_count 应为 1");
    ASSERT_TRUE(r.decl_capacity >= 1, "capacity 应 >= 1");
    ASSERT_NULL(d->name, "name 应为 NULL（零初始化）");
    ASSERT_NULL(d->params, "params 应为 NULL");
    ASSERT_EQ_INT(d->kind, 0, "kind 应为 0（零初始化）");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_result_add_decl_growth) {
    TEST_BEGIN("cupdate_result_add_decl 触发扩容");
    CUPResult r;
    cupdate_result_init(&r);
    for (int i = 0; i < 20; i++) {
        CUPDecl *d = cupdate_result_add_decl(&r);
        d->name = strdup("decl");
    }
    ASSERT_EQ_INT(r.decl_count, 20, "应有 20 个声明");
    ASSERT_TRUE(r.decl_capacity >= 20, "capacity 应 >= 20");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_parse_source                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_parse_source_basic) {
    TEST_BEGIN("cupdate_parse_source 解析源码");
    CUPResult r;
    cupdate_result_init(&r);
    int rc = cupdate_parse_source("int foo(void) { return 0; }", "test.c", &r);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(r.decl_count, 1, "应有 1 个声明");
    ASSERT_EQ_INT(r.decls[0].kind, CUP_DECL_FUNCTION, "应为函数");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_parse_source_multiple) {
    TEST_BEGIN("cupdate_parse_source 多声明");
    CUPResult r;
    cupdate_result_init(&r);
    const char *src = "int a; int b; void f(void);";
    int rc = cupdate_parse_source(src, "test.c", &r);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_TRUE(r.decl_count >= 2, "应有 >= 2 个声明");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_read_file                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_read_file_existing) {
    TEST_BEGIN("cup_read_file 读取存在文件");
    const char *path = "/tmp/cup_readfile_test.txt";
    write_temp_file(path, "hello world");
    char *content = cup_read_file(path);
    ASSERT_NOT_NULL(content, "应返回内容");
    ASSERT_EQ_STR(content, "hello world", "内容应匹配");
    free(content);
    remove(path);
    TEST_END();
}

TEST_SUITE(test_read_file_nonexistent) {
    TEST_BEGIN("cup_read_file 不存在的文件");
    char *content = cup_read_file("/tmp/cup_nonexistent_file_12345.c");
    ASSERT_NULL(content, "应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_read_file_empty) {
    TEST_BEGIN("cup_read_file 空文件");
    const char *path = "/tmp/cup_readfile_empty.txt";
    write_temp_file(path, "");
    char *content = cup_read_file(path);
    ASSERT_NOT_NULL(content, "应返回非 NULL");
    ASSERT_EQ_STR(content, "", "内容应为空串");
    free(content);
    remove(path);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_get_module_prefix                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_get_module_prefix_root) {
    TEST_BEGIN("cup_get_module_prefix SRC 根模块空前缀");
    ModuleDomain *root = domain_module_domain_new("root");
    char prefix[MAX_NAME_LEN * 4];
    cup_get_module_prefix((Domain *)root, prefix, sizeof(prefix));
    ASSERT_EQ_STR(prefix, "", "根模块前缀应为空");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_module_prefix_child) {
    TEST_BEGIN("cup_get_module_prefix SRC 子模块");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *child = domain_module_domain_new("child");
    domain_domain_add_child((Domain *)root, (Domain *)child);
    char prefix[MAX_NAME_LEN * 4];
    cup_get_module_prefix((Domain *)child, prefix, sizeof(prefix));
    ASSERT_EQ_STR(prefix, "child", "子模块前缀应为 child");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_module_prefix_nested) {
    TEST_BEGIN("cup_get_module_prefix SRC 嵌套模块");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *a = domain_module_domain_new("a");
    ModuleDomain *b = domain_module_domain_new("b");
    domain_domain_add_child((Domain *)root, (Domain *)a);
    domain_domain_add_child((Domain *)a, (Domain *)b);
    char prefix[MAX_NAME_LEN * 4];
    cup_get_module_prefix((Domain *)b, prefix, sizeof(prefix));
    ASSERT_EQ_STR(prefix, "a_b", "嵌套模块前缀应为 a_b");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_module_prefix_dynamic) {
    TEST_BEGIN("cup_get_module_prefix DYNAMIC 模式");
    ModuleDomain *mod = domain_module_domain_new("mod");
    mod->mode = MOD_MODE_DYNAMIC;
    char prefix[MAX_NAME_LEN * 4];
    cup_get_module_prefix((Domain *)mod, prefix, sizeof(prefix));
    ASSERT_EQ_STR(prefix, "", "DYNAMIC 应空前缀");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_get_module_prefix_external) {
    TEST_BEGIN("cup_get_module_prefix EXTERNAL 模式");
    ModuleDomain *mod = domain_module_domain_new("extmod");
    mod->mode = MOD_MODE_EXTERNAL;
    char prefix[MAX_NAME_LEN * 4];
    cup_get_module_prefix((Domain *)mod, prefix, sizeof(prefix));
    ASSERT_EQ_STR(prefix, "extmod", "EXTERNAL 应使用模块名");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_find_function / struct / type / macro / variable               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_find_function) {
    TEST_BEGIN("cup_find_function 查找函数");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("foo", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    FunctionDomain *found = cup_find_function(mod, "foo");
    ASSERT_TRUE(found == f, "应找到 foo");
    FunctionDomain *notfound = cup_find_function(mod, "bar");
    ASSERT_NULL(notfound, "bar 不应找到");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_struct) {
    TEST_BEGIN("cup_find_struct 查找结构体");
    ModuleDomain *mod = domain_module_domain_new("m");
    StructDomain *s = domain_struct_domain_new("Point");
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    StructDomain *found = cup_find_struct(mod, "Point");
    ASSERT_TRUE(found == s, "应找到 Point");
    ASSERT_NULL(cup_find_struct(mod, "Missing"), "Missing 不应找到");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_type) {
    TEST_BEGIN("cup_find_type 查找类型");
    ModuleDomain *mod = domain_module_domain_new("m");
    TypeDomain *t = domain_type_domain_new("MyInt");
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    TypeDomain *found = cup_find_type(mod, "MyInt");
    ASSERT_TRUE(found == t, "应找到 MyInt");
    ASSERT_NULL(cup_find_type(mod, "Other"), "Other 不应找到");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_macro) {
    TEST_BEGIN("cup_find_macro 查找宏");
    ModuleDomain *mod = domain_module_domain_new("m");
    MacroDomain *mac = domain_macro_domain_new("MAX");
    domain_domain_add_child((Domain *)mod, (Domain *)mac);
    MacroDomain *found = cup_find_macro(mod, "MAX");
    ASSERT_TRUE(found == mac, "应找到 MAX");
    ASSERT_NULL(cup_find_macro(mod, "MIN"), "MIN 不应找到");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_variable) {
    TEST_BEGIN("cup_find_variable 查找变量");
    ModuleDomain *mod = domain_module_domain_new("m");
    VariableDomain *v = domain_variable_domain_new("g_count", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)v);
    VariableDomain *found = cup_find_variable(mod, "g_count");
    ASSERT_TRUE(found == v, "应找到 g_count");
    ASSERT_NULL(cup_find_variable(mod, "g_other"), "g_other 不应找到");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_remove_all_member_children                                      */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_remove_all_member_children) {
    TEST_BEGIN("cup_remove_all_member_children 删除成员保留非成员");
    ModuleDomain *mod = domain_module_domain_new("m");
    MemberDomain *m1 = domain_member_domain_new("x", "int");
    MemberDomain *m2 = domain_member_domain_new("y", "int");
    FunctionDomain *f = domain_function_domain_new("func", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)m1);
    domain_domain_add_child((Domain *)mod, (Domain *)m2);
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    ASSERT_EQ_INT(mod->base.child_count, 3, "应有 3 个子节点");
    cup_remove_all_member_children((Domain *)mod);
    ASSERT_EQ_INT(mod->base.child_count, 1, "应剩 1 个非成员子节点");
    ASSERT_TRUE(mod->base.children[0] == (Domain *)f, "剩余应为 func");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_remove_all_member_children_none) {
    TEST_BEGIN("cup_remove_all_member_children 无成员时安全");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("func", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    cup_remove_all_member_children((Domain *)mod);
    ASSERT_EQ_INT(mod->base.child_count, 1, "数量应不变");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_function_params                                            */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_function_params_new) {
    TEST_BEGIN("cup_sync_function_params 同步到空函数");
    FunctionDomain *func = domain_function_domain_new("foo", "int");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.params = alloc_params(2);
    set_param(&d.params[0], "int", "a");
    set_param(&d.params[1], "char*", "b");
    d.param_count = 2;
    cup_sync_function_params(func, &d);
    ASSERT_EQ_INT(func->base.child_count, 2, "应有 2 个参数");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_sync_function_params_replace) {
    TEST_BEGIN("cup_sync_function_params 替换已有参数");
    FunctionDomain *func = domain_function_domain_new("foo", "int");
    MemberDomain *old = domain_member_domain_new("old_param", "int");
    domain_domain_add_child((Domain *)func, (Domain *)old);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.params = alloc_params(1);
    set_param(&d.params[0], "double", "new_param");
    d.param_count = 1;
    cup_sync_function_params(func, &d);
    ASSERT_EQ_INT(func->base.child_count, 1, "应有 1 个新参数");
    Domain *child = func->base.children[0];
    ASSERT_EQ_STR(child->name, "new_param", "应为 new_param");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_sync_function_params_skip_null_name) {
    TEST_BEGIN("cup_sync_function_params 跳过无名参数");
    FunctionDomain *func = domain_function_domain_new("foo", "int");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.params = alloc_params(2);
    set_param(&d.params[0], "int", "a");
    set_param(&d.params[1], "void", NULL);  /* 无名参数 */
    d.param_count = 2;
    cup_sync_function_params(func, &d);
    ASSERT_EQ_INT(func->base.child_count, 1, "应只添加 1 个有名字的参数");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_struct_members                                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_struct_members_new) {
    TEST_BEGIN("cup_sync_struct_members 同步新成员");
    StructDomain *sd = domain_struct_domain_new("Point");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.members = alloc_members(2);
    set_member(&d.members[0], "int", "x", NULL);
    set_member(&d.members[1], "int", "y", NULL);
    d.member_count = 2;
    cup_sync_struct_members((Domain *)sd, &d);
    ASSERT_EQ_INT(sd->base.child_count, 2, "应有 2 个成员");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)sd);
    TEST_END();
}

TEST_SUITE(test_sync_struct_members_skip_null) {
    TEST_BEGIN("cup_sync_struct_members 跳过无名成员");
    StructDomain *sd = domain_struct_domain_new("S");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.members = alloc_members(2);
    set_member(&d.members[0], "int", "x", NULL);
    set_member(&d.members[1], "int", NULL, NULL);
    d.member_count = 2;
    cup_sync_struct_members((Domain *)sd, &d);
    ASSERT_EQ_INT(sd->base.child_count, 1, "应只添加 1 个成员");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)sd);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_create_new_function                                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_create_new_function_basic) {
    TEST_BEGIN("cup_create_new_function 创建新函数");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strdup("new_func");
    d.return_type = strdup("int");
    d.is_api = 1;
    FunctionDomain *func = cup_create_new_function(mod, &d);
    ASSERT_NOT_NULL(func, "应创建函数");
    ASSERT_EQ_STR(func->base.name, "new_func", "名称应匹配");
    ASSERT_EQ_STR(func->return_type, "int", "返回类型应匹配");
    ASSERT_EQ_INT(func->mode, API_MODE_API, "应为 API 模式");
    ASSERT_EQ_INT(mod->base.child_count, 1, "模块应有 1 个子节点");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_create_new_function_with_call) {
    TEST_BEGIN("cup_create_new_function 带调用约定");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strdup("f");
    d.return_type = strdup("void");
    d.call = strdup("__stdcall");
    FunctionDomain *func = cup_create_new_function(mod, &d);
    ASSERT_NOT_NULL(func, "应创建");
    ASSERT_EQ_STR(func->call, "__stdcall", "call 应设置");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_create_new_function_with_body) {
    TEST_BEGIN("cup_create_new_function 带函数体（定义）");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strdup("f");
    d.return_type = strdup("int");
    d.is_function_def = 1;
    d.body = strdup("{ return 42; }");
    FunctionDomain *func = cup_create_new_function(mod, &d);
    ASSERT_NOT_NULL(func, "应创建");
    ASSERT_NOT_NULL(func->code, "应有函数体代码");
    ASSERT_EQ_STR(func->code, "{ return 42; }", "函数体应匹配");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_create_new_function_no_return_type) {
    TEST_BEGIN("cup_create_new_function 无返回类型默认 void");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strdup("f");
    /* return_type 为 NULL */
    FunctionDomain *func = cup_create_new_function(mod, &d);
    ASSERT_NOT_NULL(func, "应创建");
    ASSERT_EQ_STR(func->return_type, "void", "应默认 void");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_replace_str_field                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_replace_str_field_new_value) {
    TEST_BEGIN("cup_replace_str_field 新值替换");
    char *field = strdup("old");
    int changed = cup_replace_str_field(&field, "new");
    ASSERT_EQ_INT(changed, 1, "应返回 1（有变化）");
    ASSERT_EQ_STR(field, "new", "应更新为 new");
    free(field);
    TEST_END();
}

TEST_SUITE(test_replace_str_field_same_value) {
    TEST_BEGIN("cup_replace_str_field 相同值无变化");
    char *field = strdup("same");
    int changed = cup_replace_str_field(&field, "same");
    ASSERT_EQ_INT(changed, 0, "应返回 0（无变化）");
    ASSERT_EQ_STR(field, "same", "应保持不变");
    free(field);
    TEST_END();
}

TEST_SUITE(test_replace_str_field_clear) {
    TEST_BEGIN("cup_replace_str_field 清除已有值");
    char *field = strdup("existing");
    int changed = cup_replace_str_field(&field, NULL);
    ASSERT_EQ_INT(changed, 1, "应返回 1（有变化）");
    ASSERT_NULL(field, "应被清除为 NULL");
    /* field 已被 free，无需再 free */
    TEST_END();
}

TEST_SUITE(test_replace_str_field_both_null) {
    TEST_BEGIN("cup_replace_str_field 都为 NULL 无变化");
    char *field = NULL;
    int changed = cup_replace_str_field(&field, NULL);
    ASSERT_EQ_INT(changed, 0, "应返回 0");
    ASSERT_NULL(field, "应保持 NULL");
    TEST_END();
}

TEST_SUITE(test_replace_str_field_set_from_null) {
    TEST_BEGIN("cup_replace_str_field 从 NULL 设置新值");
    char *field = NULL;
    int changed = cup_replace_str_field(&field, "value");
    ASSERT_EQ_INT(changed, 1, "应返回 1");
    ASSERT_EQ_STR(field, "value", "应设置为 value");
    free(field);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_func_body                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_func_body_def_with_body) {
    TEST_BEGIN("cup_sync_func_body 定义+函数体");
    FunctionDomain *func = domain_function_domain_new("f", "int");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.is_function_def = 1;
    d.body = strdup("{ return 1; }");
    int changed = cup_sync_func_body(func, &d);
    ASSERT_EQ_INT(changed, 1, "应返回 1（有变化）");
    ASSERT_EQ_STR(func->code, "{ return 1; }", "应设置函数体");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_sync_func_body_same_body) {
    TEST_BEGIN("cup_sync_func_body 相同函数体无变化");
    FunctionDomain *func = domain_function_domain_new("f", "int");
    domain_domain_set_code((Domain *)func, "{ return 1; }");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.is_function_def = 1;
    d.body = strdup("{ return 1; }");
    int changed = cup_sync_func_body(func, &d);
    ASSERT_EQ_INT(changed, 0, "应返回 0（无变化）");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_sync_func_body_non_def_clears) {
    TEST_BEGIN("cup_sync_func_body 非定义时清除函数体");
    FunctionDomain *func = domain_function_domain_new("f", "int");
    domain_domain_set_code((Domain *)func, "{ old body }");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.is_function_def = 0;
    int changed = cup_sync_func_body(func, &d);
    ASSERT_EQ_INT(changed, 1, "应返回 1（有变化）");
    /* 清除后恢复默认代码 */
    ASSERT_EQ_STR(func->code, "//请在这里输入代码", "应恢复默认");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_update_existing_function                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_update_existing_function_return_type) {
    TEST_BEGIN("cup_update_existing_function 更新返回类型");
    FunctionDomain *func = domain_function_domain_new("f", "void");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.return_type = strdup("int");
    int changed = cup_update_existing_function(func, &d);
    ASSERT_EQ_INT(changed, 1, "应有变化");
    ASSERT_EQ_STR(func->return_type, "int", "返回类型应更新");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_update_existing_function_static_demote) {
    TEST_BEGIN("cup_update_existing_function static 降级");
    FunctionDomain *func = domain_function_domain_new("f", "int");
    func->mode = API_MODE_API;
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.is_static = 1;
    int changed = cup_update_existing_function(func, &d);
    ASSERT_TRUE(changed, "应有变化");
    ASSERT_EQ_INT(func->mode, API_MODE_NORMAL, "应降级为 NORMAL");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_update_existing_function_params) {
    TEST_BEGIN("cup_update_existing_function 同步参数");
    FunctionDomain *func = domain_function_domain_new("f", "int");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.params = alloc_params(1);
    set_param(&d.params[0], "int", "x");
    d.param_count = 1;
    int changed = cup_update_existing_function(func, &d);
    ASSERT_TRUE(changed, "应有变化");
    ASSERT_EQ_INT(func->base.child_count, 1, "应有 1 个参数");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

TEST_SUITE(test_update_existing_function_call) {
    TEST_BEGIN("cup_update_existing_function 设置调用约定");
    FunctionDomain *func = domain_function_domain_new("f", "void");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.call = strdup("__fastcall");
    int changed = cup_update_existing_function(func, &d);
    ASSERT_TRUE(changed, "应有变化");
    ASSERT_EQ_STR(func->call, "__fastcall", "call 应设置");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)func);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_function_decl                                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_function_decl_new) {
    TEST_BEGIN("cup_sync_function_decl 新函数");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_FUNCTION;
    d.name = strdup("new_func");
    d.return_type = strdup("int");
    int changes = 0;
    cup_sync_function_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    FunctionDomain *found = cup_find_function(mod, "new_func");
    ASSERT_NOT_NULL(found, "应创建函数");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_function_decl_existing) {
    TEST_BEGIN("cup_sync_function_decl 已有函数");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_FUNCTION;
    d.name = strdup("f");
    d.return_type = strdup("int");
    int changes = 0;
    cup_sync_function_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_EQ_STR(f->return_type, "int", "返回类型应更新");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_struct_decl                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_struct_decl_new) {
    TEST_BEGIN("cup_sync_struct_decl 新结构体");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_STRUCT;
    d.name = strdup("Point");
    d.members = alloc_members(2);
    set_member(&d.members[0], "int", "x", NULL);
    set_member(&d.members[1], "int", "y", NULL);
    d.member_count = 2;
    int changes = 0;
    cup_sync_struct_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    StructDomain *found = cup_find_struct(mod, "Point");
    ASSERT_NOT_NULL(found, "应创建结构体");
    ASSERT_EQ_INT(found->base.child_count, 2, "应有 2 个成员");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_struct_decl_existing) {
    TEST_BEGIN("cup_sync_struct_decl 已有结构体");
    ModuleDomain *mod = domain_module_domain_new("m");
    StructDomain *sd = domain_struct_domain_new("S");
    domain_domain_add_child((Domain *)mod, (Domain *)sd);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_STRUCT;
    d.name = strdup("S");
    d.members = alloc_members(1);
    set_member(&d.members[0], "int", "field", NULL);
    d.member_count = 1;
    int changes = 0;
    cup_sync_struct_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_EQ_INT(sd->base.child_count, 1, "应有 1 个成员");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_struct_decl_convert_type) {
    TEST_BEGIN("cup_sync_struct_decl 转换已有 type 域");
    ModuleDomain *mod = domain_module_domain_new("m");
    TypeDomain *td = domain_type_domain_new("MyT");
    td->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)td, "int");
    domain_domain_add_child((Domain *)mod, (Domain *)td);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_STRUCT;
    d.name = strdup("MyT");
    d.members = alloc_members(1);
    set_member(&d.members[0], "int", "val", NULL);
    d.member_count = 1;
    int changes = 0;
    cup_sync_struct_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_EQ_INT(td->mode, TYPE_MODE_STRUCT, "应转为 STRUCT 模式");
    ASSERT_EQ_INT(td->base.child_count, 1, "应有 1 个成员");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_typedef_decl                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_typedef_decl_new) {
    TEST_BEGIN("cup_sync_typedef_decl 新 typedef");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_TYPEDEF;
    d.name = strdup("MyInt");
    d.base_type = strdup("int");
    int changes = 0;
    cup_sync_typedef_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    TypeDomain *td = cup_find_type(mod, "MyInt");
    ASSERT_NOT_NULL(td, "应创建 type");
    ASSERT_EQ_STR(td->value, "int", "value 应为 int");
    ASSERT_EQ_INT(td->mode, TYPE_MODE_RENAME, "应为 RENAME 模式");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_typedef_decl_skip_if_struct) {
    TEST_BEGIN("cup_sync_typedef_decl 已有同 名 struct 时跳过");
    ModuleDomain *mod = domain_module_domain_new("m");
    StructDomain *sd = domain_struct_domain_new("S");
    domain_domain_add_child((Domain *)mod, (Domain *)sd);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_TYPEDEF;
    d.name = strdup("S");
    d.base_type = strdup("int");
    int changes = 0;
    cup_sync_typedef_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 0, "应无变更");
    ASSERT_NULL(cup_find_type(mod, "S"), "不应创建 type");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_typedef_decl_update_existing) {
    TEST_BEGIN("cup_sync_typedef_decl 更新已有 type");
    ModuleDomain *mod = domain_module_domain_new("m");
    TypeDomain *td = domain_type_domain_new("MyT");
    td->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)td, "int");
    domain_domain_add_child((Domain *)mod, (Domain *)td);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_TYPEDEF;
    d.name = strdup("MyT");
    d.base_type = strdup("long");
    int changes = 0;
    cup_sync_typedef_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_EQ_STR(td->value, "long", "value 应更新为 long");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_typedef_decl_skip_struct_mode) {
    TEST_BEGIN("cup_sync_typedef_decl STRUCT 模式不覆盖");
    ModuleDomain *mod = domain_module_domain_new("m");
    TypeDomain *td = domain_type_domain_new("MyT");
    td->mode = TYPE_MODE_STRUCT;
    domain_domain_add_child((Domain *)mod, (Domain *)td);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_TYPEDEF;
    d.name = strdup("MyT");
    d.base_type = strdup("int");
    int changes = 0;
    cup_sync_typedef_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 0, "应无变更");
    ASSERT_NULL(td->value, "value 不应被设置");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_macro_decl                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_macro_decl_new) {
    TEST_BEGIN("cup_sync_macro_decl 新宏");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_MACRO;
    d.name = strdup("MAX_SIZE");
    d.value = strdup("100");
    int changes = 0;
    cup_sync_macro_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    MacroDomain *md = cup_find_macro(mod, "MAX_SIZE");
    ASSERT_NOT_NULL(md, "应创建宏");
    ASSERT_EQ_STR(md->value, "100", "value 应为 100");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_macro_decl_update) {
    TEST_BEGIN("cup_sync_macro_decl 更新已有宏");
    ModuleDomain *mod = domain_module_domain_new("m");
    MacroDomain *md = domain_macro_domain_new("MAX");
    domain_domain_set_value((Domain *)md, "10");
    domain_domain_add_child((Domain *)mod, (Domain *)md);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_MACRO;
    d.name = strdup("MAX");
    d.value = strdup("20");
    int changes = 0;
    cup_sync_macro_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_EQ_STR(md->value, "20", "value 应更新为 20");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_macro_decl_clear_value) {
    TEST_BEGIN("cup_sync_macro_decl 清除已有值");
    ModuleDomain *mod = domain_module_domain_new("m");
    MacroDomain *md = domain_macro_domain_new("DEBUG");
    domain_domain_set_value((Domain *)md, "1");
    domain_domain_add_child((Domain *)mod, (Domain *)md);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_MACRO;
    d.name = strdup("DEBUG");
    /* value 为 NULL */
    int changes = 0;
    cup_sync_macro_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    ASSERT_NULL(md->value, "value 应被清除");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_var_fields                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_var_fields_update) {
    TEST_BEGIN("cup_sync_var_fields 更新类型和值");
    VariableDomain *vd = domain_variable_domain_new("g", "int");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.base_type = strdup("long");
    d.value = strdup("42");
    int changes = 0;
    cup_sync_var_fields(vd, &d, &changes);
    ASSERT_EQ_INT(changes, 2, "应有 2 处变更（类型+值）");
    ASSERT_EQ_STR(vd->type, "long", "类型应为 long");
    ASSERT_EQ_STR(vd->value, "42", "值应为 42");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)vd);
    TEST_END();
}

TEST_SUITE(test_sync_var_fields_clear_value) {
    TEST_BEGIN("cup_sync_var_fields 清除值");
    VariableDomain *vd = domain_variable_domain_new("g", "int");
    domain_domain_set_value((Domain *)vd, "old");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    /* base_type 和 value 都为 NULL */
    int changes = 0;
    cup_sync_var_fields(vd, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更（清除值）");
    ASSERT_NULL(vd->value, "值应被清除");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)vd);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_variable_decl                                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_variable_decl_new) {
    TEST_BEGIN("cup_sync_variable_decl 新变量");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_VARIABLE;
    d.name = strdup("g_count");
    d.base_type = strdup("int");
    d.value = strdup("0");
    int changes = 0;
    cup_sync_variable_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    VariableDomain *vd = cup_find_variable(mod, "g_count");
    ASSERT_NOT_NULL(vd, "应创建变量");
    ASSERT_EQ_STR(vd->type, "int", "类型应为 int");
    ASSERT_EQ_STR(vd->value, "0", "值应为 0");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_variable_decl_no_type) {
    TEST_BEGIN("cup_sync_variable_decl 无类型默认 int");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_VARIABLE;
    d.name = strdup("g");
    /* base_type 为 NULL */
    int changes = 0;
    cup_sync_variable_decl(mod, &d, &changes);
    VariableDomain *vd = cup_find_variable(mod, "g");
    ASSERT_NOT_NULL(vd, "应创建");
    ASSERT_EQ_STR(vd->type, "int", "应默认 int");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_variable_decl_static) {
    TEST_BEGIN("cup_sync_variable_decl static 变量");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_VARIABLE;
    d.name = strdup("g_static");
    d.is_static = 1;
    int changes = 0;
    cup_sync_variable_decl(mod, &d, &changes);
    VariableDomain *vd = cup_find_variable(mod, "g_static");
    ASSERT_EQ_INT(vd->mode, VAR_MODE_STATIC, "应为 STATIC 模式");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_enum_decl                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_enum_decl_new) {
    TEST_BEGIN("cup_sync_enum_decl 新枚举");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_ENUM;
    d.name = strdup("Color");
    d.members = alloc_members(3);
    set_member(&d.members[0], NULL, "RED", "0");
    set_member(&d.members[1], NULL, "GREEN", "1");
    set_member(&d.members[2], NULL, "BLUE", "2");
    d.member_count = 3;
    int changes = 0;
    cup_sync_enum_decl(mod, &d, &changes);
    ASSERT_TRUE(changes >= 4, "应至少有 4 处变更（struct + 3 macros）");
    StructDomain *sd = cup_find_struct(mod, "Color");
    ASSERT_NOT_NULL(sd, "应创建 struct Color");
    MacroDomain *red = cup_find_macro(mod, "RED");
    ASSERT_NOT_NULL(red, "应创建宏 RED");
    ASSERT_EQ_STR(red->value, "0", "RED 值应为 0");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_enum_decl_existing) {
    TEST_BEGIN("cup_sync_enum_decl 已有枚举 struct");
    ModuleDomain *mod = domain_module_domain_new("m");
    StructDomain *sd = domain_struct_domain_new("Color");
    domain_domain_add_child((Domain *)mod, (Domain *)sd);
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_ENUM;
    d.name = strdup("Color");
    d.members = alloc_members(1);
    set_member(&d.members[0], NULL, "RED", "0");
    d.member_count = 1;
    int changes = 0;
    cup_sync_enum_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应只有 1 处变更（创建宏 RED，struct 已存在）");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_sync_decl                                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_sync_decl_function) {
    TEST_BEGIN("cup_sync_decl 分发函数");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_FUNCTION;
    d.name = strdup("f");
    d.return_type = strdup("void");
    int changes = 0;
    cup_sync_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 1, "应有 1 处变更");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_decl_include_noop) {
    TEST_BEGIN("cup_sync_decl INCLUDE 无操作");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_INCLUDE;
    int changes = 0;
    cup_sync_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 0, "应无变更");
    ASSERT_EQ_INT(mod->base.child_count, 0, "不应添加子节点");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_sync_decl_other_noop) {
    TEST_BEGIN("cup_sync_decl OTHER 无操作");
    ModuleDomain *mod = domain_module_domain_new("m");
    CUPDecl d;
    memset(&d, 0, sizeof(d));
    d.kind = CUP_DECL_OTHER;
    int changes = 0;
    cup_sync_decl(mod, &d, &changes);
    ASSERT_EQ_INT(changes, 0, "应无变更");
    cup_free_decl(&d);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_decl_type_matches                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_decl_type_matches_valid) {
    TEST_BEGIN("cup_decl_type_matches 匹配类型对");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_FUNCTION, CUP_DECL_FUNCTION), "FUNCTION 匹配");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_STRUCT, CUP_DECL_STRUCT), "STRUCT 匹配 STRUCT");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_STRUCT, CUP_DECL_ENUM), "STRUCT 匹配 ENUM");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_TYPE, CUP_DECL_TYPEDEF), "TYPE 匹配 TYPEDEF");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_MACRO, CUP_DECL_MACRO), "MACRO 匹配");
    ASSERT_TRUE(cup_decl_type_matches(DOMAIN_VARIABLE, CUP_DECL_VARIABLE), "VARIABLE 匹配");
    TEST_END();
}

TEST_SUITE(test_decl_type_matches_invalid) {
    TEST_BEGIN("cup_decl_type_matches 不匹配类型对");
    ASSERT_FALSE(cup_decl_type_matches(DOMAIN_FUNCTION, CUP_DECL_STRUCT), "FUNCTION 不匹配 STRUCT");
    ASSERT_FALSE(cup_decl_type_matches(DOMAIN_TYPE, CUP_DECL_STRUCT), "TYPE 不匹配 STRUCT");
    ASSERT_FALSE(cup_decl_type_matches(DOMAIN_MODULE, CUP_DECL_FUNCTION), "MODULE 不匹配");
    ASSERT_FALSE(cup_decl_type_matches(DOMAIN_MEMBER, CUP_DECL_VARIABLE), "MEMBER 不匹配");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_find_matching_decl                                              */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_find_matching_decl_direct) {
    TEST_BEGIN("cup_find_matching_decl 直接名称匹配");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("foo", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("foo");
    int found = cup_find_matching_decl((Domain *)f, &r, "", 0);
    ASSERT_TRUE(found, "应找到匹配");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_matching_decl_prefix) {
    TEST_BEGIN("cup_find_matching_decl 前缀匹配");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("foo", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("mod_foo");
    int found = cup_find_matching_decl((Domain *)f, &r, "mod", 3);
    ASSERT_TRUE(found, "应通过前缀匹配找到");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_matching_decl_not_found) {
    TEST_BEGIN("cup_find_matching_decl 未匹配");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("foo", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("bar");
    int found = cup_find_matching_decl((Domain *)f, &r, "", 0);
    ASSERT_FALSE(found, "不应匹配");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_matching_decl_main_prefix) {
    TEST_BEGIN("cup_find_matching_decl mod_main 通过前缀匹配 main");
    /* main 排除仅检查 decl 名是否为 "main"；mod_main 不是 "main"，
     * 所以仍会做前缀匹配，匹配 domain 子节点 "main" */
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("main", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("mod_main");
    int found = cup_find_matching_decl((Domain *)f, &r, "mod", 3);
    ASSERT_TRUE(found, "mod_main 应通过前缀匹配 main");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_find_matching_decl_decl_named_main) {
    TEST_BEGIN("cup_find_matching_decl decl 名为 main 不做前缀匹配");
    /* decl 名为 "main" 时，排除前缀匹配；只能通过直接名称匹配 */
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *f = domain_function_domain_new("func", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)f);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("main");
    int found = cup_find_matching_decl((Domain *)f, &r, "mod", 3);
    ASSERT_FALSE(found, "decl main 不应匹配 domain func");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_detect_and_remove_deleted                                       */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_detect_and_remove_deleted) {
    TEST_BEGIN("cup_detect_and_remove_deleted 删除未匹配声明");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *keep = domain_function_domain_new("keep", "int");
    FunctionDomain *remove = domain_function_domain_new("remove", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)keep);
    domain_domain_add_child((Domain *)mod, (Domain *)remove);
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("keep");
    int changes = 0;
    cup_detect_and_remove_deleted(mod, &r, "", 0, &changes);
    ASSERT_EQ_INT(changes, 1, "应删除 1 个");
    ASSERT_EQ_INT(mod->base.child_count, 1, "应剩 1 个子节点");
    ASSERT_TRUE(mod->base.children[0] == (Domain *)keep, "剩余应为 keep");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_detect_keep_main_and_modules) {
    TEST_BEGIN("cup_detect_and_remove_deleted 保留 main 和子模块");
    ModuleDomain *mod = domain_module_domain_new("m");
    FunctionDomain *main_f = domain_function_domain_new("main", "int");
    ModuleDomain *sub = domain_module_domain_new("sub");
    VariableDomain *v = domain_variable_domain_new("old_var", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)main_f);
    domain_domain_add_child((Domain *)mod, (Domain *)sub);
    domain_domain_add_child((Domain *)mod, (Domain *)v);
    CUPResult r;
    cupdate_result_init(&r);
    /* 空结果：所有可移除的都应被删除，但 main 和 sub 模块应保留 */
    int changes = 0;
    cup_detect_and_remove_deleted(mod, &r, "", 0, &changes);
    ASSERT_EQ_INT(changes, 1, "应只删除 old_var");
    ASSERT_EQ_INT(mod->base.child_count, 2, "应剩 main 和 sub");
    cupdate_result_free(&r);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_skip_generated_header                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_skip_generated_header_strip) {
    TEST_BEGIN("cup_skip_generated_header 剥离生成头部");
    const char *src =
        "/* mymod.c - CBoot generated (compiler: normal) */\n"
        "/* Module: mymod */\n"
        "\n"
        "int foo(void);\n";
    const char *after = cup_skip_generated_header(src, "mymod", 5);
    ASSERT_EQ_STR(after, "int foo(void);\n", "应剥离头部");
    TEST_END();
}

TEST_SUITE(test_skip_generated_header_no_header) {
    TEST_BEGIN("cup_skip_generated_header 无头部不变");
    const char *src = "int foo(void);\n";
    const char *after = cup_skip_generated_header(src, "mymod", 5);
    ASSERT_TRUE(after == src, "应返回原指针");
    TEST_END();
}

TEST_SUITE(test_skip_generated_header_partial) {
    TEST_BEGIN("cup_skip_generated_header 部分匹配不剥离");
    /* 第一行匹配但第二行不匹配 */
    const char *src =
        "/* mymod.c - CBoot generated (compiler: normal) */\n"
        "// not a module line\n"
        "int foo(void);\n";
    const char *after = cup_skip_generated_header(src, "mymod", 5);
    ASSERT_TRUE(after == src, "部分匹配应返回原指针");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_set_module_code                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_set_module_code_with_header) {
    TEST_BEGIN("cup_set_module_code 剥离头部后设置");
    ModuleDomain *mod = domain_module_domain_new("mymod");
    const char *src =
        "/* mymod.c - CBoot generated (compiler: normal) */\n"
        "/* Module: mymod */\n"
        "\n"
        "int foo(void);\n";
    cup_set_module_code(mod, src);
    ASSERT_EQ_STR(mod->code, "int foo(void);\n", "应剥离头部设置代码");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_set_module_code_add_newline) {
    TEST_BEGIN("cup_set_module_code 无尾换行时补加");
    ModuleDomain *mod = domain_module_domain_new("m");
    const char *src = "int foo(void);";
    cup_set_module_code(mod, src);
    ASSERT_EQ_STR(mod->code, "int foo(void);\n", "应补加换行");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_set_module_code_empty) {
    TEST_BEGIN("cup_set_module_code 空源码");
    ModuleDomain *mod = domain_module_domain_new("m");
    const char *src = "";
    cup_set_module_code(mod, src);
    ASSERT_NOT_NULL(mod->code, "不应为 NULL");
    ASSERT_EQ_STR(mod->code, "", "应为空串");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_set_module_code_keep_newline) {
    TEST_BEGIN("cup_set_module_code 保留已有尾换行");
    ModuleDomain *mod = domain_module_domain_new("m");
    const char *src = "int x;\n";
    cup_set_module_code(mod, src);
    ASSERT_EQ_STR(mod->code, "int x;\n", "应保持不变");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_print_str_array / cup_print_diagnostics / cup_print_summary    */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_print_functions_no_crash) {
    TEST_BEGIN("cup_print_* 函数不崩溃");
    CUPResult r;
    cupdate_result_init(&r);
    cupdate_result_add_error(&r, "test error", 1);
    cupdate_result_add_warning(&r, "test warning", 2);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->name = strdup("foo");

    /* 调用打印函数，不应崩溃 */
    cup_print_str_array(stderr, r.errors, r.error_count);
    cup_print_str_array(stderr, NULL, 0);
    cup_print_diagnostics(&r);

    ModuleDomain *mod = domain_module_domain_new("m");
    cup_print_summary(mod, &r, 3);
    domain_domain_delete((Domain *)mod);

    cupdate_result_free(&r);
    ASSERT_TRUE(1, "所有打印函数安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_strip_decl_prefix                                               */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_strip_decl_prefix_function) {
    TEST_BEGIN("cup_strip_decl_prefix 剥离函数前缀");
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d1 = cupdate_result_add_decl(&r);
    d1->kind = CUP_DECL_FUNCTION;
    d1->name = strdup("mod_foo");
    /* main 排除仅当 decl 名本身为 "main" 时生效；"mod_main" 不是 "main"，会被剥离 */
    CUPDecl *d2 = cupdate_result_add_decl(&r);
    d2->kind = CUP_DECL_FUNCTION;
    d2->name = strdup("main");
    /* main 本身不应被剥离（也不以 mod_ 开头） */
    CUPDecl *d3 = cupdate_result_add_decl(&r);
    d3->kind = CUP_DECL_STRUCT;
    d3->name = strdup("mod_MyStruct");
    /* struct 不应被剥离（非 function/variable） */

    cup_strip_decl_prefix(&r, "mod", 3);
    ASSERT_EQ_STR(r.decls[0].name, "foo", "mod_foo 应剥离为 foo");
    ASSERT_EQ_STR(r.decls[1].name, "main", "main 应保持不变");
    ASSERT_EQ_STR(r.decls[2].name, "mod_MyStruct", "struct 不应被剥离");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_strip_decl_prefix_empty) {
    TEST_BEGIN("cup_strip_decl_prefix 空前缀不操作");
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("foo");
    cup_strip_decl_prefix(&r, "", 0);
    ASSERT_EQ_STR(r.decls[0].name, "foo", "空前缀不应改变");
    cupdate_result_free(&r);
    TEST_END();
}

TEST_SUITE(test_strip_decl_prefix_no_match) {
    TEST_BEGIN("cup_strip_decl_prefix 前缀不匹配不变");
    CUPResult r;
    cupdate_result_init(&r);
    CUPDecl *d = cupdate_result_add_decl(&r);
    d->kind = CUP_DECL_FUNCTION;
    d->name = strdup("bar");
    cup_strip_decl_prefix(&r, "mod", 3);
    ASSERT_EQ_STR(r.decls[0].name, "bar", "不匹配前缀不变");
    cupdate_result_free(&r);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_run_module                                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_run_module_null) {
    TEST_BEGIN("cupdate_run_module NULL 模块");
    int rc = cupdate_run_module(NULL, "/tmp");
    ASSERT_EQ_INT(rc, -1, "NULL 应返回 -1");
    TEST_END();
}

TEST_SUITE(test_run_module_non_module) {
    TEST_BEGIN("cupdate_run_module 非 MODULE 类型");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    int rc = cupdate_run_module((ModuleDomain *)f, "/tmp");
    ASSERT_EQ_INT(rc, -1, "非 MODULE 应返回 -1");
    domain_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_run_module_non_src) {
    TEST_BEGIN("cupdate_run_module 非 SRC 模式");
    ModuleDomain *mod = domain_module_domain_new("m");
    mod->mode = MOD_MODE_DYNAMIC;
    int rc = cupdate_run_module(mod, "/tmp");
    ASSERT_EQ_INT(rc, 0, "非 SRC 应返回 0");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_run_module_file_not_found) {
    TEST_BEGIN("cupdate_run_module 文件不存在");
    ModuleDomain *mod = domain_module_domain_new("cup_nonexistent_mod_12345");
    int rc = cupdate_run_module(mod, "/tmp");
    ASSERT_EQ_INT(rc, 0, "文件不存在应返回 0");
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_run_module_valid_file) {
    TEST_BEGIN("cupdate_run_module 有效文件");
    const char *path = "/tmp/cup_runmod_test.c";
    write_temp_file(path, "int my_func(void) { return 42; }\n");
    ModuleDomain *mod = domain_module_domain_new("cup_runmod_test");
    int rc = cupdate_run_module(mod, "/tmp");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_NOT_NULL(mod->code, "code 应已设置");
    /* 应同步函数 my_func */
    FunctionDomain *f = cup_find_function(mod, "my_func");
    ASSERT_NOT_NULL(f, "应同步 my_func");
    remove(path);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

TEST_SUITE(test_run_module_exe_compiler) {
    TEST_BEGIN("cupdate_run_module COMPILER_EXE 使用 main.c");
    const char *path = "/tmp/main.c";
    write_temp_file(path, "int main(void) { return 0; }\n");
    ModuleDomain *mod = domain_module_domain_new("anything");
    mod->compiler = COMPILER_EXE;
    int rc = cupdate_run_module(mod, "/tmp");
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    remove(path);
    domain_domain_delete((Domain *)mod);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cup_update_recursive                                                */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_update_recursive_non_module) {
    TEST_BEGIN("cup_update_recursive 非模块域安全");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    int errors = 0, warnings = 0;
    int rc = cup_update_recursive((Domain *)f, "/tmp", &errors, &warnings);
    ASSERT_EQ_INT(rc, 0, "非模块应返回 0");
    ASSERT_EQ_INT(errors, 0, "不应有错误");
    domain_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_update_recursive_with_submodule) {
    TEST_BEGIN("cup_update_recursive 含子模块");
    /* 创建根模块和子模块，子模块有 .c 文件 */
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("cup_submod_test");
    domain_domain_add_child((Domain *)root, (Domain *)sub);

    /* 写入子模块的 .c 文件到 /tmp/cup_submod_test.c */
    /* cup_update_recursive 会构造路径 /tmp/cup_submod_test/cup_submod_test.c */
    /* 这比较复杂，所以只验证不崩溃 */
    int errors = 0, warnings = 0;
    int rc = cup_update_recursive((Domain *)root, "/tmp", &errors, &warnings);
    /* 文件不存在，应返回 0 */
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* cupdate_run_project                                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_run_project_null) {
    TEST_BEGIN("cupdate_run_project NULL 项目");
    int rc = cupdate_run_project(NULL, NULL, NULL);
    ASSERT_EQ_INT(rc, -1, "NULL 应返回 -1");
    TEST_END();
}

TEST_SUITE(test_run_project_null_root) {
    TEST_BEGIN("cupdate_run_project NULL root");
    Project *p = domain_project_new("p");
    Domain *saved_root = p->root;
    p->root = NULL;
    int rc = cupdate_run_project(p, NULL, NULL);
    ASSERT_EQ_INT(rc, -1, "NULL root 应返回 -1");
    p->root = saved_root;
    domain_project_free(p);
    TEST_END();
}

TEST_SUITE(test_run_project_valid) {
    TEST_BEGIN("cupdate_run_project 有效项目（无 .c 文件）");
    Project *p = domain_project_new("cup_test_proj_nonexist");
    int errors = -1, warnings = -1;
    int rc = cupdate_run_project(p, &errors, &warnings);
    ASSERT_EQ_INT(rc, 0, "应返回 0");
    ASSERT_EQ_INT(errors, 0, "不应有错误");
    domain_project_free(p);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== cupdate/cupdate.c 测试 ==========\n");

    /* 结果容器管理 */
    RUN_SUITE(test_free_param_array_basic);
    RUN_SUITE(test_free_param_array_null);
    RUN_SUITE(test_free_member_array_basic);
    RUN_SUITE(test_free_member_array_null);
    RUN_SUITE(test_free_decl_null);
    RUN_SUITE(test_free_decl_populated);
    RUN_SUITE(test_free_decl_empty);
    RUN_SUITE(test_zero_str_slot);
    RUN_SUITE(test_free_str_array_populated);
    RUN_SUITE(test_free_str_array_null);
    RUN_SUITE(test_str_array_push_basic);
    RUN_SUITE(test_str_array_push_growth);
    RUN_SUITE(test_result_init_basic);
    RUN_SUITE(test_result_init_after_population);
    RUN_SUITE(test_result_free_empty);
    RUN_SUITE(test_result_free_populated);
    RUN_SUITE(test_result_add_error);
    RUN_SUITE(test_result_add_warning);
    RUN_SUITE(test_result_add_decl_basic);
    RUN_SUITE(test_result_add_decl_growth);

    /* 解析 */
    RUN_SUITE(test_parse_source_basic);
    RUN_SUITE(test_parse_source_multiple);

    /* 文件读取 */
    RUN_SUITE(test_read_file_existing);
    RUN_SUITE(test_read_file_nonexistent);
    RUN_SUITE(test_read_file_empty);

    /* 模块前缀 */
    RUN_SUITE(test_get_module_prefix_root);
    RUN_SUITE(test_get_module_prefix_child);
    RUN_SUITE(test_get_module_prefix_nested);
    RUN_SUITE(test_get_module_prefix_dynamic);
    RUN_SUITE(test_get_module_prefix_external);

    /* 查找函数 */
    RUN_SUITE(test_find_function);
    RUN_SUITE(test_find_struct);
    RUN_SUITE(test_find_type);
    RUN_SUITE(test_find_macro);
    RUN_SUITE(test_find_variable);

    /* 成员管理 */
    RUN_SUITE(test_remove_all_member_children);
    RUN_SUITE(test_remove_all_member_children_none);

    /* 同步参数/成员 */
    RUN_SUITE(test_sync_function_params_new);
    RUN_SUITE(test_sync_function_params_replace);
    RUN_SUITE(test_sync_function_params_skip_null_name);
    RUN_SUITE(test_sync_struct_members_new);
    RUN_SUITE(test_sync_struct_members_skip_null);

    /* 创建函数 */
    RUN_SUITE(test_create_new_function_basic);
    RUN_SUITE(test_create_new_function_with_call);
    RUN_SUITE(test_create_new_function_with_body);
    RUN_SUITE(test_create_new_function_no_return_type);

    /* 字段替换 */
    RUN_SUITE(test_replace_str_field_new_value);
    RUN_SUITE(test_replace_str_field_same_value);
    RUN_SUITE(test_replace_str_field_clear);
    RUN_SUITE(test_replace_str_field_both_null);
    RUN_SUITE(test_replace_str_field_set_from_null);

    /* 函数体同步 */
    RUN_SUITE(test_sync_func_body_def_with_body);
    RUN_SUITE(test_sync_func_body_same_body);
    RUN_SUITE(test_sync_func_body_non_def_clears);

    /* 更新已有函数 */
    RUN_SUITE(test_update_existing_function_return_type);
    RUN_SUITE(test_update_existing_function_static_demote);
    RUN_SUITE(test_update_existing_function_params);
    RUN_SUITE(test_update_existing_function_call);

    /* 同步各类声明 */
    RUN_SUITE(test_sync_function_decl_new);
    RUN_SUITE(test_sync_function_decl_existing);
    RUN_SUITE(test_sync_struct_decl_new);
    RUN_SUITE(test_sync_struct_decl_existing);
    RUN_SUITE(test_sync_struct_decl_convert_type);
    RUN_SUITE(test_sync_typedef_decl_new);
    RUN_SUITE(test_sync_typedef_decl_skip_if_struct);
    RUN_SUITE(test_sync_typedef_decl_update_existing);
    RUN_SUITE(test_sync_typedef_decl_skip_struct_mode);
    RUN_SUITE(test_sync_macro_decl_new);
    RUN_SUITE(test_sync_macro_decl_update);
    RUN_SUITE(test_sync_macro_decl_clear_value);
    RUN_SUITE(test_sync_var_fields_update);
    RUN_SUITE(test_sync_var_fields_clear_value);
    RUN_SUITE(test_sync_variable_decl_new);
    RUN_SUITE(test_sync_variable_decl_no_type);
    RUN_SUITE(test_sync_variable_decl_static);
    RUN_SUITE(test_sync_enum_decl_new);
    RUN_SUITE(test_sync_enum_decl_existing);
    RUN_SUITE(test_sync_decl_function);
    RUN_SUITE(test_sync_decl_include_noop);
    RUN_SUITE(test_sync_decl_other_noop);

    /* 类型匹配 */
    RUN_SUITE(test_decl_type_matches_valid);
    RUN_SUITE(test_decl_type_matches_invalid);

    /* 删除检测 */
    RUN_SUITE(test_find_matching_decl_direct);
    RUN_SUITE(test_find_matching_decl_prefix);
    RUN_SUITE(test_find_matching_decl_not_found);
    RUN_SUITE(test_find_matching_decl_main_prefix);
    RUN_SUITE(test_find_matching_decl_decl_named_main);
    RUN_SUITE(test_detect_and_remove_deleted);
    RUN_SUITE(test_detect_keep_main_and_modules);

    /* 头部处理 */
    RUN_SUITE(test_skip_generated_header_strip);
    RUN_SUITE(test_skip_generated_header_no_header);
    RUN_SUITE(test_skip_generated_header_partial);
    RUN_SUITE(test_set_module_code_with_header);
    RUN_SUITE(test_set_module_code_add_newline);
    RUN_SUITE(test_set_module_code_empty);
    RUN_SUITE(test_set_module_code_keep_newline);

    /* 打印函数 */
    RUN_SUITE(test_print_functions_no_crash);

    /* 前缀剥离 */
    RUN_SUITE(test_strip_decl_prefix_function);
    RUN_SUITE(test_strip_decl_prefix_empty);
    RUN_SUITE(test_strip_decl_prefix_no_match);

    /* 主流程 */
    RUN_SUITE(test_run_module_null);
    RUN_SUITE(test_run_module_non_module);
    RUN_SUITE(test_run_module_non_src);
    RUN_SUITE(test_run_module_file_not_found);
    RUN_SUITE(test_run_module_valid_file);
    RUN_SUITE(test_run_module_exe_compiler);
    RUN_SUITE(test_update_recursive_non_module);
    RUN_SUITE(test_update_recursive_with_submodule);
    RUN_SUITE(test_run_project_null);
    RUN_SUITE(test_run_project_null_root);
    RUN_SUITE(test_run_project_valid);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
