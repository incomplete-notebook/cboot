/*
 * test_core.c - domain/core/core.c 单元测试
 */
#include "test.h"
/* core.c 依赖 utils/typecheck/domain，一起包含以解决符号引用 */
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"

/* ------------------------------------------------------------------ */
/* domain_core_domain_new / 各类 _new                                  */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_domain_new_basic) {
    TEST_BEGIN("domain_new 基本创建");
    Domain *d = domain_core_domain_new(DOMAIN_MODULE, "testmod", sizeof(ModuleDomain));
    ASSERT_NOT_NULL(d, "不应为 NULL");
    ASSERT_EQ_INT(d->type, DOMAIN_MODULE, "类型应为 MODULE");
    ASSERT_EQ_STR(d->name, "testmod", "名称应为 testmod");
    ASSERT_NULL(d->parent, "parent 应为 NULL");
    ASSERT_EQ_INT(d->child_count, 0, "child_count 应为 0");
    ASSERT_EQ_INT(d->child_capacity, 8, "初始容量应为 8");
    ASSERT_NOT_NULL(d->children, "children 数组不应为 NULL");
    ASSERT_NULL(d->comment, "comment 应为 NULL");
    domain_core_domain_delete(d);
    TEST_END();
}

TEST_SUITE(test_module_domain_new) {
    TEST_BEGIN("module_domain_new 创建模块");
    ModuleDomain *m = domain_core_module_domain_new("mymod");
    ASSERT_NOT_NULL(m, "不应为 NULL");
    ASSERT_EQ_INT(m->base.type, DOMAIN_MODULE, "类型应为 MODULE");
    ASSERT_EQ_STR(m->base.name, "mymod", "名称应匹配");
    ASSERT_EQ_INT(m->mode, MOD_MODE_SRC, "默认应为 SRC 模式");
    ASSERT_EQ_INT(m->compiler, COMPILER_NORMAL, "默认应为 NORMAL 编译");
    ASSERT_NULL(m->value, "value 应为 NULL");
    ASSERT_NULL(m->code, "code 应为 NULL");
    ASSERT_EQ_INT(m->include_count, 0, "include_count 应为 0");
    ASSERT_EQ_INT(m->include_capacity, 4, "include_capacity 应为 4");
    ASSERT_NOT_NULL(m->includes, "includes 不应为 NULL");
    ASSERT_EQ_INT(m->dep_count, 0, "dep_count 应为 0");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_function_domain_new) {
    TEST_BEGIN("function_domain_new 创建函数");
    FunctionDomain *f = domain_core_function_domain_new("foo", "int");
    ASSERT_NOT_NULL(f, "不应为 NULL");
    ASSERT_EQ_INT(f->base.type, DOMAIN_FUNCTION, "类型应为 FUNCTION");
    ASSERT_EQ_STR(f->base.name, "foo", "名称应为 foo");
    ASSERT_EQ_STR(f->return_type, "int", "返回类型应为 int");
    ASSERT_EQ_INT(f->mode, API_MODE_NORMAL, "默认应为 NORMAL");
    ASSERT_NULL(f->call, "call 应为 NULL");
    ASSERT_NOT_NULL(f->code, "code 应有默认值");
    ASSERT_NULL(f->value, "value 应为 NULL");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_function_domain_new_null_return) {
    TEST_BEGIN("function_domain_new NULL 返回类型默认 void");
    FunctionDomain *f = domain_core_function_domain_new("foo", NULL);
    ASSERT_EQ_STR(f->return_type, "void", "NULL 应默认为 void");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_struct_domain_new) {
    TEST_BEGIN("struct_domain_new 创建结构体");
    StructDomain *s = domain_core_struct_domain_new("MyStruct");
    ASSERT_NOT_NULL(s, "不应为 NULL");
    ASSERT_EQ_INT(s->base.type, DOMAIN_STRUCT, "类型应为 STRUCT");
    ASSERT_EQ_STR(s->base.name, "MyStruct", "名称应匹配");
    ASSERT_EQ_INT(s->mode, API_MODE_NORMAL, "默认应为 NORMAL");
    domain_core_domain_delete((Domain *)s);
    TEST_END();
}

TEST_SUITE(test_type_domain_new) {
    TEST_BEGIN("type_domain_new 创建类型");
    TypeDomain *t = domain_core_type_domain_new("MyType");
    ASSERT_NOT_NULL(t, "不应为 NULL");
    ASSERT_EQ_INT(t->base.type, DOMAIN_TYPE, "类型应为 TYPE");
    ASSERT_EQ_INT(t->mode, TYPE_MODE_STRUCT, "默认应为 STRUCT");
    ASSERT_NULL(t->value, "value 应为 NULL");
    domain_core_domain_delete((Domain *)t);
    TEST_END();
}

TEST_SUITE(test_macro_domain_new) {
    TEST_BEGIN("macro_domain_new 创建宏");
    MacroDomain *m = domain_core_macro_domain_new("MY_MACRO");
    ASSERT_NOT_NULL(m, "不应为 NULL");
    ASSERT_EQ_INT(m->base.type, DOMAIN_MACRO, "类型应为 MACRO");
    ASSERT_EQ_INT(m->mode, API_MODE_NORMAL, "默认应为 NORMAL");
    ASSERT_NULL(m->value, "value 应为 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_variable_domain_new) {
    TEST_BEGIN("variable_domain_new 创建变量");
    VariableDomain *v = domain_core_variable_domain_new("count", "int");
    ASSERT_NOT_NULL(v, "不应为 NULL");
    ASSERT_EQ_INT(v->base.type, DOMAIN_VARIABLE, "类型应为 VARIABLE");
    ASSERT_EQ_STR(v->type, "int", "类型应为 int");
    ASSERT_EQ_INT(v->mode, VAR_MODE_NORMAL, "默认应为 NORMAL");
    ASSERT_NULL(v->value, "value 应为 NULL");
    domain_core_domain_delete((Domain *)v);
    TEST_END();
}

TEST_SUITE(test_variable_domain_new_null_type) {
    TEST_BEGIN("variable_domain_new NULL 类型默认 int");
    VariableDomain *v = domain_core_variable_domain_new("x", NULL);
    ASSERT_EQ_STR(v->type, "int", "NULL 应默认为 int");
    domain_core_domain_delete((Domain *)v);
    TEST_END();
}

TEST_SUITE(test_member_domain_new) {
    TEST_BEGIN("member_domain_new 创建成员");
    MemberDomain *m = domain_core_member_domain_new("field", "char*");
    ASSERT_NOT_NULL(m, "不应为 NULL");
    ASSERT_EQ_INT(m->base.type, DOMAIN_MEMBER, "类型应为 MEMBER");
    ASSERT_EQ_STR(m->type, "char*", "类型应为 char*");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_member_domain_new_null_type) {
    TEST_BEGIN("member_domain_new NULL 类型默认 int");
    MemberDomain *m = domain_core_member_domain_new("f", NULL);
    ASSERT_EQ_STR(m->type, "int", "NULL 应默认为 int");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_add_child / find_child                          */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_add_find_child) {
    TEST_BEGIN("add_child 和 find_child 基本操作");
    ModuleDomain *parent = domain_core_module_domain_new("parent");
    FunctionDomain *child = domain_core_function_domain_new("func", "void");
    domain_core_domain_add_child((Domain *)parent, (Domain *)child);
    ASSERT_EQ_INT(parent->base.child_count, 1, "子节点数应为 1");
    ASSERT_TRUE(child->base.parent == (Domain *)parent, "child->parent 应设置");
    Domain *found = domain_core_domain_find_child((Domain *)parent, "func");
    ASSERT_NOT_NULL(found, "应能找到");
    ASSERT_TRUE(found == (Domain *)child, "应为同一指针");
    domain_core_domain_delete((Domain *)parent);
    TEST_END();
}

TEST_SUITE(test_add_child_null) {
    TEST_BEGIN("add_child NULL 安全");
    domain_core_domain_add_child(NULL, NULL);  /* 不应崩溃 */
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_add_child((Domain *)m, NULL);
    ASSERT_EQ_INT(m->base.child_count, 0, "不应添加 NULL child");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_find_child_not_found) {
    TEST_BEGIN("find_child 未找到返回 NULL");
    ModuleDomain *m = domain_core_module_domain_new("m");
    Domain *found = domain_core_domain_find_child((Domain *)m, "nonexistent");
    ASSERT_NULL(found, "不应找到");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_find_child_null) {
    TEST_BEGIN("find_child NULL 参数安全");
    ASSERT_NULL(domain_core_domain_find_child(NULL, "x"), "NULL parent 返回 NULL");
    ModuleDomain *m = domain_core_module_domain_new("m");
    ASSERT_NULL(domain_core_domain_find_child((Domain *)m, NULL), "NULL name 返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_add_child_capacity_grow) {
    TEST_BEGIN("add_child 容量扩容");
    ModuleDomain *parent = domain_core_module_domain_new("p");
    /* 初始容量 8，添加 10 个子节点触发扩容 */
    for (int i = 0; i < 10; i++) {
        char name[16];
        snprintf(name, sizeof(name), "child%d", i);
        FunctionDomain *f = domain_core_function_domain_new(name, "void");
        domain_core_domain_add_child((Domain *)parent, (Domain *)f);
    }
    ASSERT_EQ_INT(parent->base.child_count, 10, "应有 10 个子节点");
    ASSERT_TRUE(parent->base.child_capacity >= 10, "容量应已扩容");
    Domain *found = domain_core_domain_find_child((Domain *)parent, "child9");
    ASSERT_NOT_NULL(found, "应能找到 child9");
    domain_core_domain_delete((Domain *)parent);
    TEST_END();
}

TEST_SUITE(test_find_child_by_type) {
    TEST_BEGIN("find_child_by_type 按类型查找");
    ModuleDomain *m = domain_core_module_domain_new("m");
    FunctionDomain *f = domain_core_function_domain_new("foo", "void");
    StructDomain *s = domain_core_struct_domain_new("bar");
    domain_core_domain_add_child((Domain *)m, (Domain *)f);
    domain_core_domain_add_child((Domain *)m, (Domain *)s);
    Domain *found = domain_core_domain_find_child_by_type((Domain *)m, "foo", DOMAIN_FUNCTION);
    ASSERT_TRUE(found == (Domain *)f, "应找到 function foo");
    Domain *found2 = domain_core_domain_find_child_by_type((Domain *)m, "bar", DOMAIN_STRUCT);
    ASSERT_TRUE(found2 == (Domain *)s, "应找到 struct bar");
    Domain *found3 = domain_core_domain_find_child_by_type((Domain *)m, "foo", DOMAIN_STRUCT);
    ASSERT_NULL(found3, "类型不匹配应返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_remove_child                                     */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_remove_child) {
    TEST_BEGIN("remove_child 移除子节点");
    ModuleDomain *parent = domain_core_module_domain_new("p");
    FunctionDomain *c1 = domain_core_function_domain_new("c1", "void");
    FunctionDomain *c2 = domain_core_function_domain_new("c2", "void");
    domain_core_domain_add_child((Domain *)parent, (Domain *)c1);
    domain_core_domain_add_child((Domain *)parent, (Domain *)c2);
    domain_core_domain_remove_child((Domain *)parent, (Domain *)c1);
    ASSERT_EQ_INT(parent->base.child_count, 1, "应剩 1 个子节点");
    ASSERT_TRUE(parent->base.children[0] == (Domain *)c2, "剩余应为 c2");
    domain_core_domain_delete((Domain *)c1);  /* 已从树中移除，需单独释放 */
    domain_core_domain_delete((Domain *)parent);
    TEST_END();
}

TEST_SUITE(test_remove_child_not_found) {
    TEST_BEGIN("remove_child 未找到不影响");
    ModuleDomain *parent = domain_core_module_domain_new("p");
    FunctionDomain *c1 = domain_core_function_domain_new("c1", "void");
    FunctionDomain *orphan = domain_core_function_domain_new("orphan", "void");
    domain_core_domain_add_child((Domain *)parent, (Domain *)c1);
    domain_core_domain_remove_child((Domain *)parent, (Domain *)orphan);
    ASSERT_EQ_INT(parent->base.child_count, 1, "数量应不变");
    domain_core_domain_delete((Domain *)parent);
    domain_core_domain_delete((Domain *)orphan);
    TEST_END();
}

TEST_SUITE(test_remove_child_null) {
    TEST_BEGIN("remove_child NULL 安全");
    domain_core_domain_remove_child(NULL, NULL);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_find_nearest_of_type                             */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_find_nearest_of_type) {
    TEST_BEGIN("find_nearest_of_type 向上查找");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *child = domain_core_module_domain_new("child");
    FunctionDomain *func = domain_core_function_domain_new("f", "void");
    domain_core_domain_add_child((Domain *)root, (Domain *)child);
    domain_core_domain_add_child((Domain *)child, (Domain *)func);
    /* 从 func 向上找最近的 MODULE */
    Domain *found = domain_core_domain_find_nearest_of_type((Domain *)func, DOMAIN_MODULE);
    ASSERT_TRUE(found == (Domain *)child, "最近的 MODULE 应是 child");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_find_nearest_of_type_null) {
    TEST_BEGIN("find_nearest_of_type NULL 返回 NULL");
    ASSERT_NULL(domain_core_domain_find_nearest_of_type(NULL, DOMAIN_MODULE), "NULL 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_find_nearest_of_type_self) {
    TEST_BEGIN("find_nearest_of_type 自身匹配");
    ModuleDomain *m = domain_core_module_domain_new("m");
    Domain *found = domain_core_domain_find_nearest_of_type((Domain *)m, DOMAIN_MODULE);
    ASSERT_TRUE(found == (Domain *)m, "自身应匹配");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_find_nearest_of_type_not_found) {
    TEST_BEGIN("find_nearest_of_type 未找到");
    ModuleDomain *m = domain_core_module_domain_new("m");
    Domain *found = domain_core_domain_find_nearest_of_type((Domain *)m, DOMAIN_FUNCTION);
    ASSERT_NULL(found, "无 FUNCTION 祖先应返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_find_in_tree                                     */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_find_in_tree) {
    TEST_BEGIN("find_in_tree 递归查找");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *sub = domain_core_module_domain_new("sub");
    FunctionDomain *f = domain_core_function_domain_new("target", "void");
    domain_core_domain_add_child((Domain *)root, (Domain *)sub);
    domain_core_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *found = domain_core_domain_find_in_tree((Domain *)root, DOMAIN_FUNCTION, "target");
    ASSERT_TRUE(found == (Domain *)f, "应递归找到 target");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_find_in_tree_null) {
    TEST_BEGIN("find_in_tree NULL 返回 NULL");
    ASSERT_NULL(domain_core_domain_find_in_tree(NULL, DOMAIN_FUNCTION, "x"), "NULL 应返回 NULL");
    TEST_END();
}

TEST_SUITE(test_find_in_tree_root_match) {
    TEST_BEGIN("find_in_tree 根节点匹配");
    ModuleDomain *root = domain_core_module_domain_new("root");
    Domain *found = domain_core_domain_find_in_tree((Domain *)root, DOMAIN_MODULE, "root");
    ASSERT_TRUE(found == (Domain *)root, "根节点应匹配");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_get_path                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_get_path_root) {
    TEST_BEGIN("get_path 根节点路径为 /");
    ModuleDomain *root = domain_core_module_domain_new("root");
    char *path = domain_core_domain_get_path((Domain *)root);
    ASSERT_EQ_STR(path, "/", "根路径应为 /");
    free(path);
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_path_nested) {
    TEST_BEGIN("get_path 嵌套路径");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *sub = domain_core_module_domain_new("sub");
    FunctionDomain *f = domain_core_function_domain_new("func", "void");
    domain_core_domain_add_child((Domain *)root, (Domain *)sub);
    domain_core_domain_add_child((Domain *)sub, (Domain *)f);
    char *path = domain_core_domain_get_path((Domain *)f);
    ASSERT_EQ_STR(path, "/sub/func", "嵌套路径应为 /sub/func");
    free(path);
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_get_path_null) {
    TEST_BEGIN("get_path NULL 返回 NULL");
    ASSERT_NULL(domain_core_domain_get_path(NULL), "NULL 应返回 NULL");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_is_api                                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_is_api_function_normal) {
    TEST_BEGIN("is_api 普通函数返回 0");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    ASSERT_FALSE(domain_core_domain_is_api((Domain *)f), "NORMAL 应返回 0");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_is_api_function_api) {
    TEST_BEGIN("is_api API 函数返回 1");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    f->mode = API_MODE_API;
    ASSERT_TRUE(domain_core_domain_is_api((Domain *)f), "API 应返回 1");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_is_api_struct) {
    TEST_BEGIN("is_api 结构体");
    StructDomain *s = domain_core_struct_domain_new("s");
    ASSERT_FALSE(domain_core_domain_is_api((Domain *)s), "NORMAL 应返回 0");
    s->mode = API_MODE_API;
    ASSERT_TRUE(domain_core_domain_is_api((Domain *)s), "API 应返回 1");
    domain_core_domain_delete((Domain *)s);
    TEST_END();
}

TEST_SUITE(test_is_api_null) {
    TEST_BEGIN("is_api NULL 返回 0");
    ASSERT_FALSE(domain_core_domain_is_api(NULL), "NULL 应返回 0");
    TEST_END();
}

TEST_SUITE(test_is_api_module) {
    TEST_BEGIN("is_api 模块返回 0");
    ModuleDomain *m = domain_core_module_domain_new("m");
    ASSERT_FALSE(domain_core_domain_is_api((Domain *)m), "MODULE 应返回 0");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_domain_find_api_in_submodules                           */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_find_api_in_submodules) {
    TEST_BEGIN("find_api_in_submodules 查找子模块 API");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *sub = domain_core_module_domain_new("sub");
    FunctionDomain *f = domain_core_function_domain_new("api_func", "void");
    f->mode = API_MODE_API;
    domain_core_domain_add_child((Domain *)root, (Domain *)sub);
    domain_core_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *found = domain_core_domain_find_api_in_submodules((Domain *)root, "api_func");
    ASSERT_TRUE(found == (Domain *)f, "应找到子模块中的 API 函数");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_find_api_in_submodules_non_api) {
    TEST_BEGIN("find_api_in_submodules 非 API 不返回");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *sub = domain_core_module_domain_new("sub");
    FunctionDomain *f = domain_core_function_domain_new("normal_func", "void");
    /* f->mode = API_MODE_NORMAL (默认) */
    domain_core_domain_add_child((Domain *)root, (Domain *)sub);
    domain_core_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *found = domain_core_domain_find_api_in_submodules((Domain *)root, "normal_func");
    ASSERT_NULL(found, "非 API 不应返回");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

TEST_SUITE(test_find_api_in_submodules_null) {
    TEST_BEGIN("find_api_in_submodules NULL 安全");
    ASSERT_NULL(domain_core_domain_find_api_in_submodules(NULL, "x"), "NULL scope 返回 NULL");
    ModuleDomain *m = domain_core_module_domain_new("m");
    ASSERT_NULL(domain_core_domain_find_api_in_submodules((Domain *)m, NULL), "NULL name 返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_check_api_name_conflict) {
    TEST_BEGIN("check_api_name_conflict 等价于 find_api_in_submodules");
    ModuleDomain *root = domain_core_module_domain_new("root");
    ModuleDomain *sub = domain_core_module_domain_new("sub");
    FunctionDomain *f = domain_core_function_domain_new("dup", "void");
    f->mode = API_MODE_API;
    domain_core_domain_add_child((Domain *)root, (Domain *)sub);
    domain_core_domain_add_child((Domain *)sub, (Domain *)f);
    Domain *conflict = domain_core_domain_check_api_name_conflict((Domain *)root, "dup");
    ASSERT_NOT_NULL(conflict, "应检测到冲突");
    domain_core_domain_delete((Domain *)root);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* comment 操作                                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_set_comment) {
    TEST_BEGIN("set_comment 设置主注释");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_comment((Domain *)m, "test comment");
    ASSERT_EQ_STR(m->base.comment, "test comment", "注释应设置");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_comment_overwrite) {
    TEST_BEGIN("set_comment 覆盖旧注释");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_comment((Domain *)m, "first");
    domain_core_domain_set_comment((Domain *)m, "second");
    ASSERT_EQ_STR(m->base.comment, "second", "应为 second");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_comment_null_domain) {
    TEST_BEGIN("set_comment NULL 安全");
    domain_core_domain_set_comment(NULL, "x");
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

TEST_SUITE(test_set_child_comment_new) {
    TEST_BEGIN("set_child_comment 新增");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_child_comment((Domain *)m, "func1", "comment1");
    ASSERT_EQ_INT(m->base.comment_count, 1, "应有 1 条子注释");
    ASSERT_EQ_STR(m->base.comments[0].target, "func1", "target 应为 func1");
    ASSERT_EQ_STR(m->base.comments[0].text, "comment1", "text 应为 comment1");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_child_comment_update) {
    TEST_BEGIN("set_child_comment 更新已有");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_child_comment((Domain *)m, "func1", "old");
    domain_core_domain_set_child_comment((Domain *)m, "func1", "new");
    ASSERT_EQ_INT(m->base.comment_count, 1, "数量应不变");
    ASSERT_EQ_STR(m->base.comments[0].text, "new", "text 应更新为 new");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_find_child_comment) {
    TEST_BEGIN("find_child_comment 查找");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_child_comment((Domain *)m, "func1", "c1");
    Comment *c = domain_core_find_child_comment((Domain *)m, "func1");
    ASSERT_NOT_NULL(c, "应找到");
    ASSERT_EQ_STR(c->text, "c1", "text 应为 c1");
    Comment *notfound = domain_core_find_child_comment((Domain *)m, "func2");
    ASSERT_NULL(notfound, "func2 不应找到");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_find_child_comment_null) {
    TEST_BEGIN("find_child_comment NULL 安全");
    ASSERT_NULL(domain_core_find_child_comment(NULL, "x"), "NULL domain 返回 NULL");
    ModuleDomain *m = domain_core_module_domain_new("m");
    ASSERT_NULL(domain_core_find_child_comment((Domain *)m, NULL), "NULL target 返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* value / code / call / mode 操作                                     */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_set_value_module) {
    TEST_BEGIN("set_value 模块");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_value((Domain *)m, "modval");
    ASSERT_EQ_STR(m->value, "modval", "value 应设置");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_value_function) {
    TEST_BEGIN("set_value 函数");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    domain_core_domain_set_value((Domain *)f, "funcval");
    ASSERT_EQ_STR(f->value, "funcval", "value 应设置");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_set_value_overwrite) {
    TEST_BEGIN("set_value 覆盖");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_value((Domain *)m, "v1");
    domain_core_domain_set_value((Domain *)m, "v2");
    ASSERT_EQ_STR(m->value, "v2", "应为 v2");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_value_struct_no_op) {
    TEST_BEGIN("set_value 结构体无 value 字段");
    StructDomain *s = domain_core_struct_domain_new("s");
    domain_core_domain_set_value((Domain *)s, "x");  /* 无 value 字段，不崩溃 */
    ASSERT_TRUE(1, "安全");
    domain_core_domain_delete((Domain *)s);
    TEST_END();
}

TEST_SUITE(test_set_code_module) {
    TEST_BEGIN("set_code 模块");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_code((Domain *)m, "code content");
    ASSERT_EQ_STR(m->code, "code content", "code 应设置");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_code_null_clears) {
    TEST_BEGIN("set_code NULL 清除");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_code((Domain *)m, "code");
    domain_core_domain_set_code((Domain *)m, NULL);
    ASSERT_NULL(m->code, "code 应为 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_code_function_default) {
    TEST_BEGIN("set_code 函数 NULL 恢复默认");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    domain_core_domain_set_code((Domain *)f, NULL);
    ASSERT_NOT_NULL(f->code, "NULL 应恢复默认值");
    ASSERT_EQ_STR(f->code, "//请在这里输入代码", "应为默认代码");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_set_call) {
    TEST_BEGIN("set_call 设置调用约定");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    domain_core_domain_set_call((Domain *)f, "__stdcall");
    ASSERT_EQ_STR(f->call, "__stdcall", "call 应设置");
    const char *c = domain_core_domain_get_call((Domain *)f);
    ASSERT_EQ_STR(c, "__stdcall", "get_call 应返回");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_set_call_null_clears) {
    TEST_BEGIN("set_call NULL 清除");
    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    domain_core_domain_set_call((Domain *)f, "__stdcall");
    domain_core_domain_set_call((Domain *)f, NULL);
    ASSERT_NULL(f->call, "call 应为 NULL");
    domain_core_domain_delete((Domain *)f);
    TEST_END();
}

TEST_SUITE(test_set_call_non_function) {
    TEST_BEGIN("set_call 非函数不操作");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_call((Domain *)m, "__stdcall");  /* 不崩溃 */
    ASSERT_TRUE(1, "安全");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_get_call_null) {
    TEST_BEGIN("get_call NULL 安全");
    ASSERT_NULL(domain_core_domain_get_call(NULL), "NULL 返回 NULL");
    ModuleDomain *m = domain_core_module_domain_new("m");
    ASSERT_NULL(domain_core_domain_get_call((Domain *)m), "非函数返回 NULL");
    domain_core_domain_delete((Domain *)m);
    TEST_END();
}

TEST_SUITE(test_set_mode) {
    TEST_BEGIN("set_mode 各类型");
    ModuleDomain *m = domain_core_module_domain_new("m");
    domain_core_domain_set_mode((Domain *)m, MOD_MODE_DYNAMIC);
    ASSERT_EQ_INT(m->mode, MOD_MODE_DYNAMIC, "模块模式应设置");
    domain_core_domain_delete((Domain *)m);

    FunctionDomain *f = domain_core_function_domain_new("f", "void");
    domain_core_domain_set_mode((Domain *)f, API_MODE_API);
    ASSERT_EQ_INT(f->mode, API_MODE_API, "函数模式应设置");
    domain_core_domain_delete((Domain *)f);

    TypeDomain *t = domain_core_type_domain_new("t");
    domain_core_domain_set_mode((Domain *)t, TYPE_MODE_API_STRUCT);
    ASSERT_EQ_INT(t->mode, TYPE_MODE_API_STRUCT, "类型模式应设置");
    domain_core_domain_delete((Domain *)t);
    TEST_END();
}

TEST_SUITE(test_set_mode_null) {
    TEST_BEGIN("set_mode NULL 安全");
    domain_core_domain_set_mode(NULL, 0);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* Project 操作                                                        */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_project_new) {
    TEST_BEGIN("project_new 创建项目");
    Project *p = domain_core_project_new("myproj");
    ASSERT_NOT_NULL(p, "不应为 NULL");
    ASSERT_EQ_STR(p->name, "myproj", "名称应匹配");
    ASSERT_NOT_NULL(p->root, "root 不应为 NULL");
    ASSERT_TRUE(p->current == p->root, "current 应指向 root");
    ASSERT_EQ_INT(p->has_generated, 0, "has_generated 应为 0");
    ASSERT_EQ_INT(p->dep_count, 0, "dep_count 应为 0");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_free_null) {
    TEST_BEGIN("project_free NULL 安全");
    domain_core_project_free(NULL);
    ASSERT_TRUE(1, "NULL 安全");
    TEST_END();
}

TEST_SUITE(test_project_add_dependency) {
    TEST_BEGIN("project_add_dependency 添加依赖");
    Project *p = domain_core_project_new("p");
    domain_core_project_add_dependency(p, "mod_a", "mod_b", "b.cboot");
    ASSERT_EQ_INT(p->dep_count, 1, "应有 1 条依赖");
    ASSERT_EQ_STR(p->dependencies[0].importer, "mod_a", "importer 应为 mod_a");
    ASSERT_EQ_STR(p->dependencies[0].source, "mod_b", "source 应为 mod_b");
    ASSERT_EQ_STR(p->dependencies[0].cboot_file, "b.cboot", "cboot_file 应匹配");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_add_dependency_null_cboot) {
    TEST_BEGIN("project_add_dependency NULL cboot_file");
    Project *p = domain_core_project_new("p");
    domain_core_project_add_dependency(p, "a", "b", NULL);
    ASSERT_EQ_INT(p->dep_count, 1, "应有 1 条依赖");
    ASSERT_NULL(p->dependencies[0].cboot_file, "cboot_file 应为 NULL");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_add_dependency_null_safe) {
    TEST_BEGIN("project_add_dependency NULL 安全");
    domain_core_project_add_dependency(NULL, "a", "b", NULL);
    Project *p = domain_core_project_new("p");
    domain_core_project_add_dependency(p, NULL, "b", NULL);
    ASSERT_EQ_INT(p->dep_count, 0, "NULL importer 不添加");
    domain_core_project_add_dependency(p, "a", NULL, NULL);
    ASSERT_EQ_INT(p->dep_count, 0, "NULL source 不添加");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_has_dependency) {
    TEST_BEGIN("project_has_dependency 检查");
    Project *p = domain_core_project_new("p");
    domain_core_project_add_dependency(p, "a", "b", NULL);
    ASSERT_TRUE(domain_core_project_has_dependency(p, "a", "b"), "应存在");
    ASSERT_FALSE(domain_core_project_has_dependency(p, "a", "c"), "a->c 不应存在");
    ASSERT_FALSE(domain_core_project_has_dependency(p, "x", "b"), "x->b 不应存在");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_has_dependency_null) {
    TEST_BEGIN("project_has_dependency NULL 安全");
    ASSERT_FALSE(domain_core_project_has_dependency(NULL, "a", "b"), "NULL 返回 0");
    Project *p = domain_core_project_new("p");
    ASSERT_FALSE(domain_core_project_has_dependency(p, NULL, "b"), "NULL importer 返回 0");
    ASSERT_FALSE(domain_core_project_has_dependency(p, "a", NULL), "NULL source 返回 0");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_self) {
    TEST_BEGIN("project_would_cycle 自环检测");
    Project *p = domain_core_project_new("p");
    ASSERT_TRUE(domain_core_project_would_cycle(p, "a", "a"), "自引用应成环");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_direct) {
    TEST_BEGIN("project_would_cycle 直接环");
    Project *p = domain_core_project_new("p");
    /* 已有 a 依赖 b；若 b 再依赖 a（would_cycle(b,a)）则成环 */
    domain_core_project_add_dependency(p, "a", "b", NULL);
    ASSERT_TRUE(domain_core_project_would_cycle(p, "b", "a"), "b->a 加上已有 a->b 应成环");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_no_cycle) {
    TEST_BEGIN("project_would_cycle 无环");
    Project *p = domain_core_project_new("p");
    domain_core_project_add_dependency(p, "a", "b", NULL);
    /* c 依赖 b，b 不依赖 c，无环 */
    ASSERT_FALSE(domain_core_project_would_cycle(p, "c", "b"), "c->b 不应成环");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_transitive) {
    TEST_BEGIN("project_would_cycle 传递环");
    Project *p = domain_core_project_new("p");
    /* 已有 a->b, b->c；若 c 依赖 a（would_cycle(c,a)）则成环 */
    domain_core_project_add_dependency(p, "a", "b", NULL);
    domain_core_project_add_dependency(p, "b", "c", NULL);
    ASSERT_TRUE(domain_core_project_would_cycle(p, "c", "a"), "c->a 应成环（传递）");
    domain_core_project_free(p);
    TEST_END();
}

TEST_SUITE(test_project_would_cycle_null) {
    TEST_BEGIN("project_would_cycle NULL 安全");
    ASSERT_FALSE(domain_core_project_would_cycle(NULL, "a", "b"), "NULL 返回 0");
    Project *p = domain_core_project_new("p");
    ASSERT_FALSE(domain_core_project_would_cycle(p, NULL, "b"), "NULL importer 返回 0");
    ASSERT_FALSE(domain_core_project_would_cycle(p, "a", NULL), "NULL source 返回 0");
    domain_core_project_free(p);
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* domain_core_is_builtin_type                                         */
/* ------------------------------------------------------------------ */
TEST_SUITE(test_is_builtin_type_basic) {
    TEST_BEGIN("is_builtin_type 基本类型");
    ASSERT_TRUE(domain_core_is_builtin_type("int"), "int 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("char"), "char 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("void"), "void 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("float"), "float 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("double"), "double 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("long"), "long 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("short"), "short 应为内置");
    TEST_END();
}

TEST_SUITE(test_is_builtin_type_pointer) {
    TEST_BEGIN("is_builtin_type 指针类型");
    ASSERT_TRUE(domain_core_is_builtin_type("int*"), "int* 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("char *"), "char * 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("void**"), "void** 应为内置");
    TEST_END();
}

TEST_SUITE(test_is_builtin_type_compound) {
    TEST_BEGIN("is_builtin_type 复合类型");
    ASSERT_TRUE(domain_core_is_builtin_type("unsigned int"), "unsigned int 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("unsigned long"), "unsigned long 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("long long"), "long long 应为内置");
    ASSERT_TRUE(domain_core_is_builtin_type("long double"), "long double 应为内置");
    TEST_END();
}

TEST_SUITE(test_is_builtin_type_not_builtin) {
    TEST_BEGIN("is_builtin_type 非内置");
    ASSERT_FALSE(domain_core_is_builtin_type("MyStruct"), "自定义结构体非内置");
    ASSERT_FALSE(domain_core_is_builtin_type("foo_t"), "自定义类型非内置");
    TEST_END();
}

TEST_SUITE(test_is_builtin_type_null) {
    TEST_BEGIN("is_builtin_type NULL 返回 0");
    ASSERT_FALSE(domain_core_is_builtin_type(NULL), "NULL 应返回 0");
    ASSERT_FALSE(domain_core_is_builtin_type(""), "空串应返回 0");
    TEST_END();
}

/* ------------------------------------------------------------------ */
/* 主函数                                                              */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("========== domain/core/core.c 测试 ==========\n");

    RUN_SUITE(test_domain_new_basic);
    RUN_SUITE(test_module_domain_new);
    RUN_SUITE(test_function_domain_new);
    RUN_SUITE(test_function_domain_new_null_return);
    RUN_SUITE(test_struct_domain_new);
    RUN_SUITE(test_type_domain_new);
    RUN_SUITE(test_macro_domain_new);
    RUN_SUITE(test_variable_domain_new);
    RUN_SUITE(test_variable_domain_new_null_type);
    RUN_SUITE(test_member_domain_new);
    RUN_SUITE(test_member_domain_new_null_type);
    RUN_SUITE(test_add_find_child);
    RUN_SUITE(test_add_child_null);
    RUN_SUITE(test_find_child_not_found);
    RUN_SUITE(test_find_child_null);
    RUN_SUITE(test_add_child_capacity_grow);
    RUN_SUITE(test_find_child_by_type);
    RUN_SUITE(test_remove_child);
    RUN_SUITE(test_remove_child_not_found);
    RUN_SUITE(test_remove_child_null);
    RUN_SUITE(test_find_nearest_of_type);
    RUN_SUITE(test_find_nearest_of_type_null);
    RUN_SUITE(test_find_nearest_of_type_self);
    RUN_SUITE(test_find_nearest_of_type_not_found);
    RUN_SUITE(test_find_in_tree);
    RUN_SUITE(test_find_in_tree_null);
    RUN_SUITE(test_find_in_tree_root_match);
    RUN_SUITE(test_get_path_root);
    RUN_SUITE(test_get_path_nested);
    RUN_SUITE(test_get_path_null);
    RUN_SUITE(test_is_api_function_normal);
    RUN_SUITE(test_is_api_function_api);
    RUN_SUITE(test_is_api_struct);
    RUN_SUITE(test_is_api_null);
    RUN_SUITE(test_is_api_module);
    RUN_SUITE(test_find_api_in_submodules);
    RUN_SUITE(test_find_api_in_submodules_non_api);
    RUN_SUITE(test_find_api_in_submodules_null);
    RUN_SUITE(test_check_api_name_conflict);
    RUN_SUITE(test_set_comment);
    RUN_SUITE(test_set_comment_overwrite);
    RUN_SUITE(test_set_comment_null_domain);
    RUN_SUITE(test_set_child_comment_new);
    RUN_SUITE(test_set_child_comment_update);
    RUN_SUITE(test_find_child_comment);
    RUN_SUITE(test_find_child_comment_null);
    RUN_SUITE(test_set_value_module);
    RUN_SUITE(test_set_value_function);
    RUN_SUITE(test_set_value_overwrite);
    RUN_SUITE(test_set_value_struct_no_op);
    RUN_SUITE(test_set_code_module);
    RUN_SUITE(test_set_code_null_clears);
    RUN_SUITE(test_set_code_function_default);
    RUN_SUITE(test_set_call);
    RUN_SUITE(test_set_call_null_clears);
    RUN_SUITE(test_set_call_non_function);
    RUN_SUITE(test_get_call_null);
    RUN_SUITE(test_set_mode);
    RUN_SUITE(test_set_mode_null);
    RUN_SUITE(test_project_new);
    RUN_SUITE(test_project_free_null);
    RUN_SUITE(test_project_add_dependency);
    RUN_SUITE(test_project_add_dependency_null_cboot);
    RUN_SUITE(test_project_add_dependency_null_safe);
    RUN_SUITE(test_project_has_dependency);
    RUN_SUITE(test_project_has_dependency_null);
    RUN_SUITE(test_project_would_cycle_self);
    RUN_SUITE(test_project_would_cycle_direct);
    RUN_SUITE(test_project_would_cycle_no_cycle);
    RUN_SUITE(test_project_would_cycle_transitive);
    RUN_SUITE(test_project_would_cycle_null);
    RUN_SUITE(test_is_builtin_type_basic);
    RUN_SUITE(test_is_builtin_type_pointer);
    RUN_SUITE(test_is_builtin_type_compound);
    RUN_SUITE(test_is_builtin_type_not_builtin);
    RUN_SUITE(test_is_builtin_type_null);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
