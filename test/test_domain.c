/*
 * test_domain.c - domain/domain.c 单元测试
 * domain.c 是父级转发层：所有函数仅调用 domain_core_* 实现。
 * 本测试验证转发层的接口正确性，并通过公共 API 验证端到端行为。
 */
#include "test.h"
/* 转发层依赖 core 实现 + utils/typecheck */
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"

/* ------------------------------------------------------------------ */
/* domain_domain_new / 各类 *_new 转发                                 */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_domain_new_forward) {
    TEST_BEGIN("domain_new 转发");
    Domain *d = domain_domain_new(DOMAIN_MODULE, "m", sizeof(ModuleDomain));
    ASSERT_NOT_NULL(d, "不应为 NULL");
    ASSERT_EQ_INT(d->type, DOMAIN_MODULE, "类型应为 MODULE");
    ASSERT_EQ_STR(d->name, "m", "名称应匹配");
    domain_domain_delete(d);
    TEST_END();
}

TEST_SUITE(test_module_domain_new_forward) {
    TEST_BEGIN("module_domain_new 转发");
    ModuleDomain *m = domain_module_domain_new("mod");
    ASSERT_NOT_NULL(m, "不应为 NULL");
    ASSERT_EQ_STR(m->base.name, "mod", "名称应匹配");
    ASSERT_EQ_INT(m->mode, MOD_MODE_SRC, "默认 SRC");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_function_domain_new_forward) {
    TEST_BEGIN("function_domain_new 转发");
    FunctionDomain *f = domain_function_domain_new("fn", "int");
    ASSERT_EQ_STR(f->return_type, "int", "返回类型应转发");
    domain_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_struct_domain_new_forward) {
    TEST_BEGIN("struct_domain_new 转发");
    StructDomain *s = domain_struct_domain_new("S");
    ASSERT_EQ_INT(s->base.type, DOMAIN_STRUCT, "类型 STRUCT");
    domain_domain_delete((Domain *)s);
    TEST_END();
}

TEST_SUITE(test_type_domain_new_forward) {
    TEST_BEGIN("type_domain_new 转发");
    TypeDomain *t = domain_type_domain_new("T");
    ASSERT_EQ_INT(t->base.type, DOMAIN_TYPE, "类型 TYPE");
    domain_domain_delete((Domain *)t);
    TEST_END();
}

TEST_SUITE(test_macro_domain_new_forward) {
    TEST_BEGIN("macro_domain_new 转发");
    MacroDomain *m = domain_macro_domain_new("M");
    ASSERT_EQ_INT(m->base.type, DOMAIN_MACRO, "类型 MACRO");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_variable_domain_new_forward) {
    TEST_BEGIN("variable_domain_new 转发");
    VariableDomain *v = domain_variable_domain_new("var", "char*");
    ASSERT_EQ_STR(v->type, "char*", "类型应转发");
    domain_domain_delete((Domain *)v);
    TEST_END();
}

TEST_SUITE(test_member_domain_new_forward) {
    TEST_BEGIN("member_domain_new 转发");
    MemberDomain *m = domain_member_domain_new("mem", "int");
    ASSERT_EQ_STR(m->type, "int", "类型应转发");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 树操作转发                                                          */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_add_find_child_forward) {
    TEST_BEGIN("add_child/find_child 转发");
    ModuleDomain *p = domain_module_domain_new("p");
    FunctionDomain *c = domain_function_domain_new("c", "void");
    domain_domain_add_child((Domain *)p, (Domain *)c);
    ASSERT_EQ_INT(p->base.child_count, 1, "应有 1 子节点");
    Domain *found = domain_domain_find_child((Domain *)p, "c");
    ASSERT_TRUE(found == (Domain *)c, "应转发找到");
    domain_domain_delete((Domain *)p);
    TEST_END();
}

TEST_SUITE(test_find_child_by_type_forward) {
    TEST_BEGIN("find_child_by_type 转发");
    ModuleDomain *p = domain_module_domain_new("p");
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_add_child((Domain *)p, (Domain *)s);
    Domain *found = domain_domain_find_child_by_type((Domain *)p, "S", DOMAIN_STRUCT);
    ASSERT_TRUE(found == (Domain *)s, "应按类型找到");
    domain_domain_delete((Domain *)p);
    TEST_END();
}

TEST_SUITE(test_remove_child_forward) {
    TEST_BEGIN("remove_child 转发");
    ModuleDomain *p = domain_module_domain_new("p");
    FunctionDomain *c = domain_function_domain_new("c", "void");
    domain_domain_add_child((Domain *)p, (Domain *)c);
    domain_domain_remove_child((Domain *)p, (Domain *)c);
    ASSERT_EQ_INT(p->base.child_count, 0, "应已移除");
    domain_domain_delete((Domain *)c);
    domain_domain_delete((Domain *)p);
    TEST_END();
}

TEST_SUITE(test_find_nearest_of_type_forward) {
    TEST_BEGIN("find_nearest_of_type 转发");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)root, (Domain *)sub);
    Domain *found = domain_domain_find_nearest_of_type((Domain *)sub, DOMAIN_MODULE);
    ASSERT_TRUE(found == (Domain *)sub, "应转发找到自身");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_find_in_tree_forward) {
    TEST_BEGIN("find_in_tree 转发");
    ModuleDomain *root = domain_module_domain_new("root");
    FunctionDomain *f = domain_function_domain_new("target", "void");
    domain_domain_add_child((Domain *)root, (Domain *)f);
    Domain *found = domain_domain_find_in_tree((Domain *)root, DOMAIN_FUNCTION, "target");
    ASSERT_TRUE(found == (Domain *)f, "应转发递归找到");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_path_forward) {
    TEST_BEGIN("get_path 转发");
    ModuleDomain *root = domain_module_domain_new("root");
    char *path = domain_domain_get_path((Domain *)root);
    ASSERT_EQ_STR(path, "/", "根路径应为 /");
    free(path);
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_is_api_forward) {
    TEST_BEGIN("is_api 转发");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    ASSERT_FALSE(domain_domain_is_api((Domain *)f), "NORMAL 应返回 0");
    f->mode = API_MODE_API;
    ASSERT_TRUE(domain_domain_is_api((Domain *)f), "API 应返回 1");
    domain_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_find_api_in_submodules_forward) {
    TEST_BEGIN("find_api_in_submodules 转发");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("sub");
    FunctionDomain *f = domain_function_domain_new("api", "void");
    f->mode = API_MODE_API;
    domain_domain_add_child((Domain *)root, (Domain *)sub);
    domain_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *found = domain_domain_find_api_in_submodules((Domain *)root, "api");
    ASSERT_TRUE(found == (Domain *)f, "应转发找到 API");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_check_api_name_conflict_forward) {
    TEST_BEGIN("check_api_name_conflict 转发");
    ModuleDomain *root = domain_module_domain_new("root");
    ModuleDomain *sub = domain_module_domain_new("sub");
    FunctionDomain *f = domain_function_domain_new("dup", "void");
    f->mode = API_MODE_API;
    domain_domain_add_child((Domain *)root, (Domain *)sub);
    domain_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *c = domain_domain_check_api_name_conflict((Domain *)root, "dup");
    ASSERT_NOT_NULL(c, "应转发检测到冲突");
    domain_domain_delete((Domain *)root);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* comment 操作转发                                                    */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_set_comment_forward) {
    TEST_BEGIN("set_comment 转发");
    ModuleDomain *m = domain_module_domain_new("m");
    domain_domain_set_comment((Domain *)m, "comment");
    ASSERT_EQ_STR(m->base.comment, "comment", "注释应转发设置");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_child_comment_forward) {
    TEST_BEGIN("set_child_comment 转发");
    ModuleDomain *m = domain_module_domain_new("m");
    domain_domain_set_child_comment((Domain *)m, "f", "c");
    Comment *c = domain_find_child_comment((Domain *)m, "f");
    ASSERT_NOT_NULL(c, "应转发找到");
    ASSERT_EQ_STR(c->text, "c", "text 应匹配");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* value/code/call/mode 转发                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_set_value_forward) {
    TEST_BEGIN("set_value 转发");
    ModuleDomain *m = domain_module_domain_new("m");
    domain_domain_set_value((Domain *)m, "val");
    ASSERT_EQ_STR(m->value, "val", "value 应转发");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_code_forward) {
    TEST_BEGIN("set_code 转发");
    ModuleDomain *m = domain_module_domain_new("m");
    domain_domain_set_code((Domain *)m, "code");
    ASSERT_EQ_STR(m->code, "code", "code 应转发");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_call_forward) {
    TEST_BEGIN("set_call/get_call 转发");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    domain_domain_set_call((Domain *)f, "__stdcall");
    ASSERT_EQ_STR(f->call, "__stdcall", "call 应转发");
    ASSERT_EQ_STR(domain_domain_get_call((Domain *)f), "__stdcall", "get_call 应转发");
    domain_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_set_mode_forward) {
    TEST_BEGIN("set_mode 转发");
    ModuleDomain *m = domain_module_domain_new("m");
    domain_domain_set_mode((Domain *)m, MOD_MODE_DYNAMIC);
    ASSERT_EQ_INT(m->mode, MOD_MODE_DYNAMIC, "mode 应转发");
    domain_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* Project 操作转发                                                    */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_project_new_forward) {
    TEST_BEGIN("project_new 转发");
    Project *p = domain_project_new("proj");
    ASSERT_EQ_STR(p->name, "proj", "名称应转发");
    ASSERT_NOT_NULL(p->root, "root 不应为 NULL");
    domain_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_add_dependency_forward) {
    TEST_BEGIN("project_add_dependency 转发");
    Project *p = domain_project_new("p");
    domain_project_add_dependency(p, "a", "b", "b.cboot");
    ASSERT_EQ_INT(p->dep_count, 1, "应有 1 依赖");
    ASSERT_TRUE(domain_project_has_dependency(p, "a", "b"), "应转发存在");
    domain_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_forward) {
    TEST_BEGIN("project_would_cycle 转发");
    Project *p = domain_project_new("p");
    ASSERT_TRUE(domain_project_would_cycle(p, "a", "a"), "自环应转发检测");
    domain_project_free(p);
    TEST_END();
}

TEST_SUITE(test_is_builtin_type_forward) {
    TEST_BEGIN("is_builtin_type 转发");
    ASSERT_TRUE(domain_is_builtin_type("int"), "int 应转发内置");
    ASSERT_FALSE(domain_is_builtin_type("MyType"), "自定义应转发非内置");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== domain/domain.c 转发层测试 ==========\n");

    RUN_SUITE(test_domain_new_forward);
    RUN_SUITE(test_module_domain_new_forward);
    RUN_SUITE(test_function_domain_new_forward);
    RUN_SUITE(test_struct_domain_new_forward);
    RUN_SUITE(test_type_domain_new_forward);
    RUN_SUITE(test_macro_domain_new_forward);
    RUN_SUITE(test_variable_domain_new_forward);
    RUN_SUITE(test_member_domain_new_forward);
    RUN_SUITE(test_add_find_child_forward);
    RUN_SUITE(test_find_child_by_type_forward);
    RUN_SUITE(test_remove_child_forward);
    RUN_SUITE(test_find_nearest_of_type_forward);
    RUN_SUITE(test_find_in_tree_forward);
    RUN_SUITE(test_get_path_forward);
    RUN_SUITE(test_is_api_forward);
    RUN_SUITE(test_find_api_in_submodules_forward);
    RUN_SUITE(test_check_api_name_conflict_forward);
    RUN_SUITE(test_set_comment_forward);
    RUN_SUITE(test_set_child_comment_forward);
    RUN_SUITE(test_set_value_forward);
    RUN_SUITE(test_set_code_forward);
    RUN_SUITE(test_set_call_forward);
    RUN_SUITE(test_set_mode_forward);
    RUN_SUITE(test_project_new_forward);
    RUN_SUITE(test_project_add_dependency_forward);
    RUN_SUITE(test_project_would_cycle_forward);
    RUN_SUITE(test_is_builtin_type_forward);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
