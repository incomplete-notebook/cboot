/*
 * test_generator.c - generator/generator.c 单元测试
 *
 * 通过 #include "generator/generator.c" 暴露 static 函数进行测试。
 * generator.c 依赖: utils/utils.c, domain/core/core.c, domain/domain.c
 * generator.c 调用 docgen_generate_module_docs（仅此一个 docgen 函数），
 * 因 generator.c 与 docgen.c 都各自定义了 static docgen_is_api，无法同时
 * #include，故对 docgen_generate_module_docs 提供 mock（用计数器验证调用）。
 * typecheck.c 由 core.c 的 domain_core_is_builtin_type 间接依赖，一并引入。
 * generator.c 不调用 cupdate/parser/commands，无需引入或 mock。
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "generator/generator.c"

#include <unistd.h>

/* ------------------------------------------------------------------ */
/* 全局状态（cboot.h 中 extern 声明，此处定义）                        */
/* ------------------------------------------------------------------ */
Project *g_proj   = NULL;
RunMode  g_mode   = MODE_INTERACTIVE;
int      g_force  = 0;
int      g_running = 0;
int      g_skip_gen = 0;
char     g_script_dir[MAX_PATH_LEN] = ".";

/* ------------------------------------------------------------------ */
/* mock: docgen_generate_module_docs                                   */
/* ------------------------------------------------------------------ */
static int g_docgen_call_count = 0;
void docgen_generate_module_docs(Domain *mod, const char *dir) {
    (void)mod;
    (void)dir;
    g_docgen_call_count++;
}

/* ------------------------------------------------------------------ */
/* 测试辅助                                                            */
/* ------------------------------------------------------------------ */

/* 读取整个文件到 malloc 缓冲区（调用者负责 free） */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) sz = 0;
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static int contains(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    return strstr(hay, needle) != NULL;
}
static int contains_not(const char *hay, const char *needle) {
    if (!hay || !needle) return 1;
    return strstr(hay, needle) == NULL;
}

#define ASSERT_CONTAINS(c, n, m)     ASSERT_TRUE(contains(c, n), m)
#define ASSERT_NOT_CONTAINS(c, n, m) ASSERT_TRUE(contains_not(c, n), m)

/* 每个测试用例独立作用域，避免同一 suite 内多块变量重定义 */
#define CASE(name)   TEST_BEGIN(name); {
#define ENDCASE      TEST_END(); }

/* 创建（或重建）一个干净的临时目录 */
static void make_tmp_dir(const char *path) {
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'", path, path);
    system(cmd);
}

#define TMP_FILE "/tmp/test_gen_tmp.txt"

static FILE *open_tmp(void) { return fopen(TMP_FILE, "w"); }
static char *close_and_read(FILE *f) { fclose(f); return read_file(TMP_FILE); }

/* chdir 辅助：g_orig_cwd 在 main 中保存一次真实工作目录 */
static char g_orig_cwd[MAX_PATH_LEN];
static void enter_tmp(const char *path) {
    make_tmp_dir(path);
    chdir(path);
}
static void leave_tmp(const char *path) {
    chdir(g_orig_cwd);
    char cmd[MAX_PATH_LEN * 2];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    system(cmd);
}

/* 把模块挂到项目根下（使 parent 链有效，prefix 才能正确生成） */
static ModuleDomain *make_mod_in_proj(Project *p, const char *name) {
    ModuleDomain *m = domain_module_domain_new(name);
    domain_domain_add_child(p->root, (Domain *)m);
    return m;
}

/* 直接设置 ModuleDomain 的 dependency / include（无公共 API） */
static void mod_add_dep(ModuleDomain *md, const char *dep) {
    md->dependencies[md->dep_count++] = utils_str_dup(dep);
}
static void mod_add_include(ModuleDomain *md, const char *inc) {
    md->includes[md->include_count++] = utils_str_dup(inc);
}

/* 手动向 proj 添加一条 in 导入记录（无公共 API） */
static void proj_add_import(Project *proj, const char *path) {
    if (proj->import_count >= proj->import_capacity) {
        proj->import_capacity = (proj->import_capacity == 0) ? 8 : proj->import_capacity * 2;
        proj->imported_projects = (char **)realloc(proj->imported_projects,
                                                    sizeof(char *) * proj->import_capacity);
    }
    proj->imported_projects[proj->import_count++] = utils_str_dup(path);
}

/* ================================================================== */
/* 1. docgen_is_api (static, generator.c 内的副本)                     */
/* ================================================================== */
TEST_SUITE(test_gen_is_api) {
    CASE("is_api: NULL 返回 0");
    ASSERT_EQ_INT(docgen_is_api(NULL), 0, "NULL 应返回 0");
    ENDCASE;

    CASE("is_api: FUNCTION API/NORMAL");
    FunctionDomain *f = domain_function_domain_new("f", "int");
    ASSERT_EQ_INT(docgen_is_api((Domain *)f), 0, "FUNCTION NORMAL 非 API");
    f->mode = API_MODE_API;
    ASSERT_EQ_INT(docgen_is_api((Domain *)f), 1, "FUNCTION API 是 API");
    domain_domain_delete((Domain *)f);
    ENDCASE;

    CASE("is_api: STRUCT API/NORMAL");
    StructDomain *s = domain_struct_domain_new("S");
    ASSERT_EQ_INT(docgen_is_api((Domain *)s), 0, "STRUCT NORMAL 非 API");
    s->mode = API_MODE_API;
    ASSERT_EQ_INT(docgen_is_api((Domain *)s), 1, "STRUCT API 是 API");
    domain_domain_delete((Domain *)s);
    ENDCASE;

    CASE("is_api: TYPE 四种 mode");
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_RENAME;      ASSERT_EQ_INT(docgen_is_api((Domain *)t), 0, "RENAME 非 API");
    t->mode = TYPE_MODE_STRUCT;      ASSERT_EQ_INT(docgen_is_api((Domain *)t), 0, "STRUCT 非 API");
    t->mode = TYPE_MODE_API_RENAME;  ASSERT_EQ_INT(docgen_is_api((Domain *)t), 1, "API_RENAME 是 API");
    t->mode = TYPE_MODE_API_STRUCT;  ASSERT_EQ_INT(docgen_is_api((Domain *)t), 1, "API_STRUCT 是 API");
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("is_api: MACRO API/NORMAL");
    MacroDomain *m = domain_macro_domain_new("M");
    ASSERT_EQ_INT(docgen_is_api((Domain *)m), 0, "MACRO NORMAL 非 API");
    m->mode = API_MODE_API;
    ASSERT_EQ_INT(docgen_is_api((Domain *)m), 1, "MACRO API 是 API");
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("is_api: 其它类型走 default 返回 0");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = domain_module_domain_new("mod");
    VariableDomain *v = domain_variable_domain_new("v", "int");
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    ASSERT_EQ_INT(docgen_is_api((Domain *)mod), 0, "MODULE 非 API");
    ASSERT_EQ_INT(docgen_is_api((Domain *)v), 0, "VARIABLE 非 API");
    ASSERT_EQ_INT(docgen_is_api((Domain *)mb), 0, "MEMBER 非 API");
    ASSERT_EQ_INT(docgen_is_api((Domain *)p->root), 0, "root MODULE 非 API");
    domain_domain_delete((Domain *)v);
    domain_domain_delete((Domain *)mb);
    domain_domain_delete((Domain *)mod);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 2. generator_compiler_mode_str (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_compiler_mode_str) {
    CASE("compiler_mode_str: 全部分支");
    ASSERT_EQ_STR(generator_compiler_mode_str(COMPILER_EXE), "exe", "EXE");
    ASSERT_EQ_STR(generator_compiler_mode_str(COMPILER_SL), "sl", "SL");
    ASSERT_EQ_STR(generator_compiler_mode_str(COMPILER_DL), "dl", "DL");
    ASSERT_EQ_STR(generator_compiler_mode_str(COMPILER_NORMAL), "normal", "NORMAL");
    ASSERT_EQ_STR(generator_compiler_mode_str((CompilerMode)999), "normal", "default→normal");
    ENDCASE;
}

/* ================================================================== */
/* 3. generator_get_module_prefix (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_get_module_prefix) {
    CASE("get_module_prefix: 顶层 SRC 模块用自身名");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "domain");
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)mod, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "domain", "顶层模块前缀=模块名");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: 嵌套 SRC 模块用下划线连接");
    Project *p = domain_project_new("p");
    ModuleDomain *a = make_mod_in_proj(p, "a");
    ModuleDomain *b = domain_module_domain_new("b");
    domain_domain_add_child((Domain *)a, (Domain *)b);
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)b, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "a_b", "嵌套模块前缀=a_b");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: 三层嵌套");
    Project *p = domain_project_new("p");
    ModuleDomain *a = make_mod_in_proj(p, "a");
    ModuleDomain *b = domain_module_domain_new("b");
    ModuleDomain *c = domain_module_domain_new("c");
    domain_domain_add_child((Domain *)a, (Domain *)b);
    domain_domain_add_child((Domain *)b, (Domain *)c);
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)c, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "a_b_c", "三层嵌套前缀=a_b_c");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: STATIC 模式返回空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_STATIC;
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)mod, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "", "STATIC 前缀为空");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: DYNAMIC 模式返回空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_DYNAMIC;
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)mod, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "", "DYNAMIC 前缀为空");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: EXTERNAL 模式用模块名");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "ext");
    mod->mode = MOD_MODE_EXTERNAL;
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)mod, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "ext", "EXTERNAL 前缀=模块名");
    domain_project_free(p);
    ENDCASE;

    CASE("get_module_prefix: 孤立模块(无 parent)返回空");
    ModuleDomain *mod = domain_module_domain_new("lonely");
    char buf[MAX_NAME_LEN * 4];
    generator_get_module_prefix((Domain *)mod, buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "", "无 parent 的模块前缀为空");
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 4. generator_make_abs_name (static)                                */
/* ================================================================== */
TEST_SUITE(test_gen_make_abs_name) {
    CASE("make_abs_name: main 始终保持原名");
    char buf[MAX_NAME_LEN * 5];
    generator_make_abs_name("mod", "main", buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "main", "main 不加前缀");
    ENDCASE;

    CASE("make_abs_name: 空 prefix(DYNAMIC) 用原始名");
    char buf[MAX_NAME_LEN * 5];
    generator_make_abs_name("", "foo", buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "foo", "空前缀用原名");
    ENDCASE;

    CASE("make_abs_name: 正常 prefix_name");
    char buf[MAX_NAME_LEN * 5];
    generator_make_abs_name("dom", "foo", buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "dom_foo", "正常前缀拼接");
    ENDCASE;

    CASE("make_abs_name: main 即使空 prefix 仍为 main");
    char buf[MAX_NAME_LEN * 5];
    generator_make_abs_name("", "main", buf, sizeof(buf));
    ASSERT_EQ_STR(buf, "main", "main 优先级最高");
    ENDCASE;
}

/* ================================================================== */
/* 5. generator_extract_struct_name (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_extract_struct_name) {
    char sname[MAX_NAME_LEN];

    CASE("extract_struct_name: NULL 返回 0");
    ASSERT_EQ_INT(generator_extract_struct_name(NULL, sname, sizeof(sname)), 0, "NULL");
    ENDCASE;

    CASE("extract_struct_name: 非 struct 前缀返回 0");
    ASSERT_EQ_INT(generator_extract_struct_name("int", sname, sizeof(sname)), 0, "int");
    ASSERT_EQ_INT(generator_extract_struct_name("Foo *", sname, sizeof(sname)), 0, "Foo *");
    ASSERT_EQ_INT(generator_extract_struct_name("const char*", sname, sizeof(sname)), 0, "const char*");
    ENDCASE;

    CASE("extract_struct_name: struct Name");
    ASSERT_EQ_INT(generator_extract_struct_name("struct Foo", sname, sizeof(sname)), 1, "struct Foo");
    ASSERT_EQ_STR(sname, "Foo", "提取 Foo");
    ENDCASE;

    CASE("extract_struct_name: struct Name * 去掉尾部 * 和空格");
    ASSERT_EQ_INT(generator_extract_struct_name("struct Foo *", sname, sizeof(sname)), 1, "struct Foo *");
    ASSERT_EQ_STR(sname, "Foo", "去掉 *");
    ASSERT_EQ_INT(generator_extract_struct_name("struct Bar  *  ", sname, sizeof(sname)), 1, "struct Bar  *  ");
    ASSERT_EQ_STR(sname, "Bar", "去掉 * 和空格");
    ENDCASE;

    CASE("extract_struct_name: 仅 'struct ' 后为空返回 0");
    ASSERT_EQ_INT(generator_extract_struct_name("struct ", sname, sizeof(sname)), 0, "struct 后空");
    ASSERT_EQ_INT(generator_extract_struct_name("struct *", sname, sizeof(sname)), 0, "struct * 全是 *");
    ENDCASE;
}

/* ================================================================== */
/* 6. generator_fwd_list_contains (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_fwd_list_contains) {
    CASE("fwd_list_contains: 空/命中/未命中");
    char arr[4][MAX_NAME_LEN];
    strncpy(arr[0], "Foo", MAX_NAME_LEN);
    strncpy(arr[1], "Bar", MAX_NAME_LEN);
    ASSERT_EQ_INT(generator_fwd_list_contains(arr, 0, "Foo"), 0, "空列表");
    ASSERT_EQ_INT(generator_fwd_list_contains(arr, 2, "Foo"), 1, "命中 Foo");
    ASSERT_EQ_INT(generator_fwd_list_contains(arr, 2, "Bar"), 1, "命中 Bar");
    ASSERT_EQ_INT(generator_fwd_list_contains(arr, 2, "Baz"), 0, "未命中 Baz");
    ENDCASE;
}

/* ================================================================== */
/* 7. generator_defined_in_module (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_defined_in_module) {
    CASE("defined_in_module: API struct/type 命中，非 API 不命中");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s_api = domain_struct_domain_new("Pub");
    s_api->mode = API_MODE_API;
    StructDomain *s_priv = domain_struct_domain_new("Priv");
    TypeDomain *t_api = domain_type_domain_new("H");
    t_api->mode = TYPE_MODE_API_RENAME;
    TypeDomain *t_priv = domain_type_domain_new("PrivT");
    t_priv->mode = TYPE_MODE_RENAME;
    domain_domain_add_child((Domain *)mod, (Domain *)s_api);
    domain_domain_add_child((Domain *)mod, (Domain *)s_priv);
    domain_domain_add_child((Domain *)mod, (Domain *)t_api);
    domain_domain_add_child((Domain *)mod, (Domain *)t_priv);

    ASSERT_EQ_INT(generator_defined_in_module((Domain *)mod, "Pub"), 1, "API struct 命中");
    ASSERT_EQ_INT(generator_defined_in_module((Domain *)mod, "H"), 1, "API type 命中");
    ASSERT_EQ_INT(generator_defined_in_module((Domain *)mod, "Priv"), 0, "非 API struct 不命中");
    ASSERT_EQ_INT(generator_defined_in_module((Domain *)mod, "PrivT"), 0, "非 API type 不命中");
    ASSERT_EQ_INT(generator_defined_in_module((Domain *)mod, "Nope"), 0, "不存在不命中");
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 8. generator_collect_func_types (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_collect_func_types) {
    CASE("collect_func_types: 返回类型 + 参数类型");
    FunctionDomain *fn = domain_function_domain_new("f", "struct Foo *");
    MemberDomain *p1 = domain_member_domain_new("a", "int");
    MemberDomain *p2 = domain_member_domain_new("b", "struct Bar *");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    domain_domain_add_child((Domain *)fn, (Domain *)p2);
    const char *out[64];
    int n = generator_collect_func_types(fn, (Domain *)fn, out, 64);
    ASSERT_EQ_INT(n, 3, "1 返回 + 2 参数");
    ASSERT_EQ_STR(out[0], "struct Foo *", "返回类型");
    ASSERT_EQ_STR(out[1], "int", "参数 a");
    ASSERT_EQ_STR(out[2], "struct Bar *", "参数 b");
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("collect_func_types: 无参数仅返回类型");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    const char *out[64];
    int n = generator_collect_func_types(fn, (Domain *)fn, out, 64);
    ASSERT_EQ_INT(n, 1, "仅返回类型");
    ASSERT_EQ_STR(out[0], "void", "void");
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 9. generator_fwd_try_add (static)                                  */
/* ================================================================== */
TEST_SUITE(test_gen_fwd_try_add) {
    CASE("fwd_try_add: 新增/去重/已定义/非 struct");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s = domain_struct_domain_new("Defined");
    s->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)s);

    char fwd[32][MAX_NAME_LEN];
    int count = 0;

    /* struct Extern — 不在模块中，应加入 */
    generator_fwd_try_add("struct Extern *", (Domain *)mod, fwd, &count, 32);
    ASSERT_EQ_INT(count, 1, "新增 Extern");
    ASSERT_EQ_STR(fwd[0], "Extern", "fwd[0]=Extern");

    /* 重复 Extern — 去重，不增加 */
    generator_fwd_try_add("struct Extern", (Domain *)mod, fwd, &count, 32);
    ASSERT_EQ_INT(count, 1, "去重不增加");

    /* struct Defined — 模块中已定义，跳过 */
    generator_fwd_try_add("struct Defined *", (Domain *)mod, fwd, &count, 32);
    ASSERT_EQ_INT(count, 1, "模块已定义跳过");

    /* int — 非 struct，跳过 */
    generator_fwd_try_add("int", (Domain *)mod, fwd, &count, 32);
    ASSERT_EQ_INT(count, 1, "非 struct 跳过");

    /* struct Another — 新增 */
    generator_fwd_try_add("struct Another", (Domain *)mod, fwd, &count, 32);
    ASSERT_EQ_INT(count, 2, "新增 Another");
    ASSERT_EQ_STR(fwd[1], "Another", "fwd[1]=Another");

    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 10. generator_write_function_params (static)                       */
/* ================================================================== */
TEST_SUITE(test_gen_write_function_params) {
    CASE("write_function_params: 多参数");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    MemberDomain *p1 = domain_member_domain_new("a", "int");
    MemberDomain *p2 = domain_member_domain_new("b", "const char*");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    domain_domain_add_child((Domain *)fn, (Domain *)p2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_function_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int a, const char* b", "多参数列表");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_function_params: 无参数");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    FILE *f = open_tmp();
    generator_write_function_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无参数输出为空");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_function_params: 跳过非 MEMBER 子节点");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    VariableDomain *v = domain_variable_domain_new("local", "int");
    MemberDomain *p = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)fn, (Domain *)v);
    domain_domain_add_child((Domain *)fn, (Domain *)p);
    FILE *f = open_tmp();
    generator_write_function_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int x", "仅输出 MEMBER");
    ASSERT_NOT_CONTAINS(c, "local", "跳过 VARIABLE");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 11. generator_write_func_signature (static)                        */
/* ================================================================== */
TEST_SUITE(test_gen_write_func_signature) {
    CASE("write_func_signature: 无 call");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_func_signature(fn, "abs_f", f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int abs_f(", "无 call 签名");
    ASSERT_NOT_CONTAINS(c, "__stdcall", "无 call 不出现调用约定");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_func_signature: 有 call");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    domain_domain_set_call((Domain *)fn, "__stdcall");
    FILE *f = open_tmp();
    generator_write_func_signature(fn, "abs_f", f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void __stdcall abs_f(", "有 call 签名");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 12. generator_write_func_vars (static)                             */
/* ================================================================== */
TEST_SUITE(test_gen_write_func_vars) {
    CASE("write_func_vars: 无变量不输出");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    FILE *f = open_tmp();
    generator_write_func_vars((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无变量输出为空");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_func_vars: static/normal + 注释 + 初值");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    VariableDomain *v1 = domain_variable_domain_new("count", "int");
    v1->mode = VAR_MODE_STATIC;
    domain_domain_set_value((Domain *)v1, "0");
    VariableDomain *v2 = domain_variable_domain_new("buf", "char*");
    domain_domain_set_comment((Domain *)v2, "缓冲区");
    domain_domain_add_child((Domain *)fn, (Domain *)v1);
    domain_domain_add_child((Domain *)fn, (Domain *)v2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_func_vars((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "    static int count = 0;", "static 带初值");
    ASSERT_CONTAINS(c, "    // 缓冲区", "变量注释");
    ASSERT_CONTAINS(c, "    char* buf;", "普通变量");
    ASSERT_CONTAINS(c, "\n\n", "末尾空行");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 13. generator_write_struct_members (static)                        */
/* ================================================================== */
TEST_SUITE(test_gen_write_struct_members) {
    CASE("write_struct_members: 带/不带注释");
    StructDomain *s = domain_struct_domain_new("S");
    MemberDomain *m1 = domain_member_domain_new("x", "int");
    MemberDomain *m2 = domain_member_domain_new("name", "char*");
    domain_domain_set_comment((Domain *)m2, "名称");
    domain_domain_add_child((Domain *)s, (Domain *)m1);
    domain_domain_add_child((Domain *)s, (Domain *)m2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_struct_members((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "    int x;", "无注释成员");
    ASSERT_CONTAINS(c, "    char* name;  // 名称", "带注释成员");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;

    CASE("write_struct_members: 跳过非 MEMBER");
    StructDomain *s = domain_struct_domain_new("S");
    FunctionDomain *fn = domain_function_domain_new("skip", "void");
    MemberDomain *m = domain_member_domain_new("a", "int");
    domain_domain_add_child((Domain *)s, (Domain *)fn);
    domain_domain_add_child((Domain *)s, (Domain *)m);
    FILE *f = open_tmp();
    generator_write_struct_members((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int a;", "仅 MEMBER");
    ASSERT_NOT_CONTAINS(c, "skip", "跳过非 MEMBER");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;
}

/* ================================================================== */
/* 14. generator_write_c_includes (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_write_c_includes) {
    CASE("write_c_includes: 自身/子模块/依赖/额外 include");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *child = domain_module_domain_new("child");
    domain_domain_add_child((Domain *)mod, (Domain *)child);
    mod_add_dep(mod, "dep");
    mod_add_include(mod, "<stdio.h>");

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_c_includes((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "#include \"mod.h\"", "自身头");
    ASSERT_CONTAINS(c, "#include \"child/child.h\"", "子模块头");
    ASSERT_CONTAINS(c, "#include \"dep.h\"", "依赖头");
    ASSERT_CONTAINS(c, "#include <stdio.h>", "额外 include");
    ASSERT_TRUE(c[strlen(c) - 1] == '\n', "末尾换行");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_includes: 仅自身头");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FILE *f = open_tmp();
    generator_write_c_includes((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "#include \"mod.h\"", "仅自身头");
    ASSERT_NOT_CONTAINS(c, "child", "无子模块");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 15. generator_write_c_defs (static)                                */
/* ================================================================== */
TEST_SUITE(test_gen_write_c_defs) {
    CASE("write_c_defs: 宏有值/无值/带注释");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    MacroDomain *m1 = domain_macro_domain_new("MAX");
    domain_domain_set_value((Domain *)m1, "100");
    MacroDomain *m2 = domain_macro_domain_new("DEBUG");
    domain_domain_set_comment((Domain *)m2, "调试开关");
    MacroDomain *m3 = domain_macro_domain_new("EMPTY");
    domain_domain_add_child((Domain *)mod, (Domain *)m1);
    domain_domain_add_child((Domain *)mod, (Domain *)m2);
    domain_domain_add_child((Domain *)mod, (Domain *)m3);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_c_defs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "#define MAX 100", "带值宏");
    ASSERT_CONTAINS(c, "// 调试开关", "宏注释");
    ASSERT_CONTAINS(c, "#define DEBUG", "无值宏");
    ASSERT_CONTAINS(c, "#define EMPTY", "空宏");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_defs: 无宏输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FILE *f = open_tmp();
    generator_write_c_defs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无宏输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 16. generator_write_c_types (static)                               */
/* ================================================================== */
TEST_SUITE(test_gen_write_c_types) {
    CASE("write_c_types: 非 API struct 加注释");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s = domain_struct_domain_new("Priv");
    MemberDomain *mx = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    domain_domain_add_child((Domain *)mod, (Domain *)s);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_c_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "/* non-API struct - static to avoid symbol collision */", "非 API 注释");
    ASSERT_CONTAINS(c, "typedef struct Priv {", "struct 定义");
    ASSERT_CONTAINS(c, "    int x;", "成员");
    ASSERT_CONTAINS(c, "} Priv;", "struct 结束");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_types: API struct 不加注释");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s = domain_struct_domain_new("Pub");
    s->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    FILE *f = open_tmp();
    generator_write_c_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "non-API struct", "API struct 无注释");
    ASSERT_CONTAINS(c, "typedef struct Pub {", "struct 定义");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_types: TYPE RENAME 有值");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    TypeDomain *t = domain_type_domain_new("MyInt");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)t, "long");
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    FILE *f = open_tmp();
    generator_write_c_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef long MyInt;", "rename 类型");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_types: TYPE RENAME 无值默认 int");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_RENAME;
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    FILE *f = open_tmp();
    generator_write_c_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef int T;", "无值默认 int");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_types: TYPE STRUCT 模式");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    TypeDomain *t = domain_type_domain_new("Op");
    t->mode = TYPE_MODE_STRUCT;
    MemberDomain *mk = domain_member_domain_new("kind", "int");
    domain_domain_add_child((Domain *)t, (Domain *)mk);
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    FILE *f = open_tmp();
    generator_write_c_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef struct Op {", "struct 形式 type");
    ASSERT_CONTAINS(c, "    int kind;", "type 成员");
    ASSERT_CONTAINS(c, "} Op;", "type 结束");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 17. generator_write_c_variables (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_write_c_variables) {
    CASE("write_c_variables: NORMAL/STATIC + 值 + abs_name");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    VariableDomain *v1 = domain_variable_domain_new("count", "int");
    domain_domain_set_value((Domain *)v1, "0");
    VariableDomain *v2 = domain_variable_domain_new("buf", "char*");
    v2->mode = VAR_MODE_STATIC;
    domain_domain_add_child((Domain *)mod, (Domain *)v1);
    domain_domain_add_child((Domain *)mod, (Domain *)v2);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_c_variables((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int mod_count = 0;", "abs_name + 值");
    ASSERT_CONTAINS(c, "static char* mod_buf;", "static + abs_name");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_variables: DYNAMIC 模式用原名");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_DYNAMIC;
    VariableDomain *v = domain_variable_domain_new("count", "int");
    domain_domain_add_child((Domain *)mod, (Domain *)v);
    FILE *f = open_tmp();
    generator_write_c_variables((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int count;", "DYNAMIC 用原名");
    ASSERT_NOT_CONTAINS(c, "lib_count", "DYNAMIC 不加前缀");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 18. generator_write_c_functions (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_write_c_functions) {
    CASE("write_c_functions: code/value/comment/abs_name");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("init", "int");
    fn->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)fn, "初始化");
    domain_domain_set_value((Domain *)fn, "return 0 业务");
    domain_domain_set_code((Domain *)fn, "    return 0;");
    MemberDomain *param = domain_member_domain_new("cfg", "void*");
    domain_domain_add_child((Domain *)fn, (Domain *)param);
    domain_domain_add_child((Domain *)mod, (Domain *)fn);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_c_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "// 初始化", "注释");
    ASSERT_CONTAINS(c, "// 业务逻辑: return 0 业务", "value 业务逻辑");
    ASSERT_CONTAINS(c, "int mod_init(void* cfg) {", "abs_name 签名 + 参数");
    ASSERT_CONTAINS(c, "    return 0;\n}", "code 体");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_functions: code 为 NULL 输出 TODO");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    fn->code = NULL;  /* 直接置空，绕过 set_code 的默认值 */
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    FILE *f = open_tmp();
    generator_write_c_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "    // TODO: implement", "NULL code 输出 TODO");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_functions: main 保持原名");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("main", "int");
    domain_domain_set_code((Domain *)fn, "    return 0;");
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    FILE *f = open_tmp();
    generator_write_c_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int main(", "main 不加前缀");
    ASSERT_NOT_CONTAINS(c, "mod_main", "main 无前缀");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_c_functions: 有 call 约定");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("cb", "void");
    domain_domain_set_call((Domain *)fn, "__stdcall");
    domain_domain_set_code((Domain *)fn, "    ;");
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    FILE *f = open_tmp();
    generator_write_c_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void __stdcall mod_cb(", "call 约定");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 19. generator_write_h_guard_begin / end (static)                   */
/* ================================================================== */
TEST_SUITE(test_gen_write_h_guard) {
    CASE("write_h_guard_begin/end: 大写化 + 包裹");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    char guard[MAX_NAME_LEN * 2];
    generator_write_h_guard_begin(f, "my_mod", guard, sizeof(guard));
    fprintf(f, "/* body */\n");
    generator_write_h_guard_end(f, guard);
    char *c = close_and_read(f);
    ASSERT_EQ_STR(guard, "MY_MOD_H", "guard 大写");
    ASSERT_CONTAINS(c, "/* my_mod.h - CBoot generated", "头注释");
    ASSERT_CONTAINS(c, "#ifndef MY_MOD_H", "#ifndef");
    ASSERT_CONTAINS(c, "#define MY_MOD_H", "#define");
    ASSERT_CONTAINS(c, "#endif /* MY_MOD_H */", "#endif");
    free(c);
    ENDCASE;
}

/* ================================================================== */
/* 20. generator_write_dllexport_macro (static)                       */
/* ================================================================== */
TEST_SUITE(test_gen_write_dllexport_macro) {
    CASE("write_dllexport_macro: 完整 WIN32 块");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_dllexport_macro(f, "mymod");
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "#ifdef _WIN32", "WIN32 条件");
    ASSERT_CONTAINS(c, "#  ifdef mymod_EXPORTS", "EXPORTS 宏");
    ASSERT_CONTAINS(c, "#    define mymod_API __declspec(dllexport)", "dllexport");
    ASSERT_CONTAINS(c, "#    define mymod_API __declspec(dllimport)", "dllimport");
    ASSERT_CONTAINS(c, "#  define mymod_API", "非 WIN32 空 API");
    free(c);
    ENDCASE;
}

/* ================================================================== */
/* 21. generator_write_h_api_macros (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_write_h_api_macros) {
    CASE("write_h_api_macros: API 宏有/无值 + 跳过私有");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    MacroDomain *ma = domain_macro_domain_new("MAX");
    ma->mode = API_MODE_API;
    domain_domain_set_value((Domain *)ma, "256");
    MacroDomain *mb = domain_macro_domain_new("FLAG");
    mb->mode = API_MODE_API;
    MacroDomain *mp = domain_macro_domain_new("INTERNAL");
    mp->mode = API_MODE_NORMAL;
    domain_domain_add_child((Domain *)mod, (Domain *)ma);
    domain_domain_add_child((Domain *)mod, (Domain *)mb);
    domain_domain_add_child((Domain *)mod, (Domain *)mp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_h_api_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "#define MAX 256", "API 带值宏");
    ASSERT_CONTAINS(c, "#define FLAG", "API 无值宏");
    ASSERT_NOT_CONTAINS(c, "INTERNAL", "私有宏跳过");
    ASSERT_TRUE(c[strlen(c) - 1] == '\n', "有 API 时末尾空行");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_api_macros: 无 API 宏输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FILE *f = open_tmp();
    generator_write_h_api_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无 API 宏输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 22. generator_write_api_rename_type (static)                       */
/* ================================================================== */
TEST_SUITE(test_gen_write_api_rename_type) {
    CASE("write_api_rename_type: API_RENAME 有值");
    TypeDomain *t = domain_type_domain_new("H");
    t->mode = TYPE_MODE_API_RENAME;
    domain_domain_set_value((Domain *)t, "void*");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_api_rename_type(t, (Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef void* H;", "rename 类型");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("write_api_rename_type: API_RENAME 无值默认 int");
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_RENAME;
    FILE *f = open_tmp();
    generator_write_api_rename_type(t, (Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef int T;", "无值默认 int");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("write_api_rename_type: API_STRUCT 前向声明");
    TypeDomain *t = domain_type_domain_new("Op");
    t->mode = TYPE_MODE_API_STRUCT;
    FILE *f = open_tmp();
    generator_write_api_rename_type(t, (Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef struct Op Op;", "struct 前向声明");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;
}

/* ================================================================== */
/* 23. generator_write_h_api_types (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_write_h_api_types) {
    CASE("write_h_api_types: API struct 完整定义 + API type");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s = domain_struct_domain_new("Point");
    s->mode = API_MODE_API;
    MemberDomain *mx = domain_member_domain_new("x", "int");
    MemberDomain *my = domain_member_domain_new("y", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    domain_domain_add_child((Domain *)s, (Domain *)my);
    TypeDomain *t = domain_type_domain_new("Handle");
    t->mode = TYPE_MODE_API_RENAME;
    domain_domain_set_value((Domain *)t, "void*");
    StructDomain *sp = domain_struct_domain_new("Priv");
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    domain_domain_add_child((Domain *)mod, (Domain *)sp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_h_api_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef struct Point {", "API struct 定义");
    ASSERT_CONTAINS(c, "    int x;", "struct 成员 x");
    ASSERT_CONTAINS(c, "    int y;", "struct 成员 y");
    ASSERT_CONTAINS(c, "} Point;", "struct 结束");
    ASSERT_CONTAINS(c, "typedef void* Handle;", "API type");
    ASSERT_NOT_CONTAINS(c, "Priv", "私有 struct 跳过");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_api_types: 无 API 类型输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *sp = domain_struct_domain_new("Priv");
    domain_domain_add_child((Domain *)mod, (Domain *)sp);
    FILE *f = open_tmp();
    generator_write_h_api_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无 API 类型输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 24. generator_write_h_fwd_decls (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_write_h_fwd_decls) {
    CASE("write_h_fwd_decls: 外部 struct 参数生成前向声明");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("open", "struct Extern *");
    fn->mode = API_MODE_API;
    MemberDomain *param = domain_member_domain_new("h", "struct Extern *");
    domain_domain_add_child((Domain *)fn, (Domain *)param);
    domain_domain_add_child((Domain *)mod, (Domain *)fn);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_h_fwd_decls((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "/* Forward declarations for struct types used in API signatures */", "前向声明注释");
    ASSERT_CONTAINS(c, "typedef struct Extern Extern;", "Extern 前向声明");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_fwd_decls: 模块内已定义的 struct 不前向声明");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    StructDomain *s = domain_struct_domain_new("Defined");
    s->mode = API_MODE_API;
    FunctionDomain *fn = domain_function_domain_new("get", "struct Defined *");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    FILE *f = open_tmp();
    generator_write_h_fwd_decls((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "Forward declarations", "已定义不前向声明");
    ASSERT_TRUE(c != NULL && c[0] == '\0', "输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_fwd_decls: 无 API 函数输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("priv", "void");
    /* NORMAL 模式 */
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    FILE *f = open_tmp();
    generator_write_h_fwd_decls((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无 API 函数输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 25. generator_write_h_api_functions (static)                       */
/* ================================================================== */
TEST_SUITE(test_gen_write_h_api_functions) {
    CASE("write_h_api_functions: NORMAL 模块 + abs_name + 跳过私有");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    mod->compiler = COMPILER_NORMAL;
    FunctionDomain *fa = domain_function_domain_new("init", "int");
    fa->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)fa, "初始化");
    MemberDomain *param = domain_member_domain_new("cfg", "void*");
    domain_domain_add_child((Domain *)fa, (Domain *)param);
    FunctionDomain *fp = domain_function_domain_new("helper", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_h_api_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "// 初始化", "注释");
    ASSERT_CONTAINS(c, "int mod_init(void* cfg);", "abs_name 声明");
    ASSERT_NOT_CONTAINS(c, "helper", "私有函数跳过");
    ASSERT_NOT_CONTAINS(c, "mod_API", "NORMAL 无 _API 前缀");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_api_functions: DL 模块加 _API 前缀");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_DL;
    FunctionDomain *fa = domain_function_domain_new("open", "int");
    fa->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_write_h_api_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "lib_API int lib_open();", "DL 加 _API 前缀");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_api_functions: 有 call 约定");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fa = domain_function_domain_new("cb", "void");
    fa->mode = API_MODE_API;
    domain_domain_set_call((Domain *)fa, "__stdcall");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    FILE *f = open_tmp();
    generator_write_h_api_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void __stdcall mod_cb();", "call 约定");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_h_api_functions: DL + call 同时");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_DL;
    FunctionDomain *fa = domain_function_domain_new("cb", "void");
    fa->mode = API_MODE_API;
    domain_domain_set_call((Domain *)fa, "__stdcall");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    FILE *f = open_tmp();
    generator_write_h_api_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "lib_API void __stdcall lib_cb();", "DL + call");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 26. generator_generate_mod_c (static)                              */
/* ================================================================== */
TEST_SUITE(test_gen_generate_mod_c) {
    CASE("generate_mod_c: NORMAL 生成 <mod>.c");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    domain_domain_set_code((Domain *)fn, "    return 0;");
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    const char *dir = "/tmp/test_gen_modc1";
    make_tmp_dir(dir);
    generator_generate_mod_c((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modc1/mod.c");
    ASSERT_NOT_NULL(c, "生成 mod.c");
    ASSERT_CONTAINS(c, "/* mod.c - CBoot generated (compiler: normal) */", "头注释 normal");
    ASSERT_CONTAINS(c, "#include \"mod.h\"", "include 自身头");
    ASSERT_CONTAINS(c, "int mod_f(", "函数 abs_name");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modc1");
    ENDCASE;

    CASE("generate_mod_c: EXE 生成 main.c");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "app");
    mod->compiler = COMPILER_EXE;
    FunctionDomain *fn = domain_function_domain_new("main", "int");
    domain_domain_set_code((Domain *)fn, "    return 0;");
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    const char *dir = "/tmp/test_gen_modc2";
    make_tmp_dir(dir);
    generator_generate_mod_c((Domain *)mod, dir);
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_modc2/main.c"), "生成 main.c");
    ASSERT_FALSE(utils_file_exists("/tmp/test_gen_modc2/app.c"), "EXE 不生成 app.c");
    char *c = read_file("/tmp/test_gen_modc2/main.c");
    ASSERT_CONTAINS(c, "/* app.c - CBoot generated (compiler: exe) */", "头注释 exe");
    ASSERT_CONTAINS(c, "int main(", "main 原名");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modc2");
    ENDCASE;

    CASE("generate_mod_c: 有 code 字段直接使用原始代码");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    domain_domain_set_code((Domain *)mod, "int custom_func(void) { return 42; }");
    const char *dir = "/tmp/test_gen_modc3";
    make_tmp_dir(dir);
    generator_generate_mod_c((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modc3/mod.c");
    ASSERT_NOT_NULL(c, "生成 mod.c");
    ASSERT_CONTAINS(c, "/* mod.c - CBoot generated", "头注释");
    ASSERT_CONTAINS(c, "int custom_func(void) { return 42; }", "原始代码");
    ASSERT_NOT_CONTAINS(c, "#include \"mod.h\"", "code 模式不输出 include");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modc3");
    ENDCASE;

    CASE("generate_mod_c: code 为空字符串走正常流程");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    domain_domain_set_code((Domain *)mod, "");
    const char *dir = "/tmp/test_gen_modc4";
    make_tmp_dir(dir);
    generator_generate_mod_c((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modc4/mod.c");
    ASSERT_CONTAINS(c, "#include \"mod.h\"", "空 code 走正常流程");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modc4");
    ENDCASE;

    CASE("generate_mod_c: DL 模式输出 dllexport 宏");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_DL;
    const char *dir = "/tmp/test_gen_modc5";
    make_tmp_dir(dir);
    generator_generate_mod_c((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modc5/lib.c");
    ASSERT_CONTAINS(c, "compiler: dl", "头注释 dl");
    ASSERT_CONTAINS(c, "#ifdef _WIN32", "dllexport 宏块");
    ASSERT_CONTAINS(c, "lib_EXPORTS", "EXPORTS 宏");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modc5");
    ENDCASE;
}

/* ================================================================== */
/* 27. generator_generate_mod_h (static)                              */
/* ================================================================== */
TEST_SUITE(test_gen_generate_mod_h) {
    CASE("generate_mod_h: 完整 .h 文件 (NORMAL)");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fa = domain_function_domain_new("init", "int");
    fa->mode = API_MODE_API;
    MemberDomain *param = domain_member_domain_new("cfg", "struct Extern *");
    domain_domain_add_child((Domain *)fa, (Domain *)param);
    MacroDomain *ma = domain_macro_domain_new("MAX");
    ma->mode = API_MODE_API;
    domain_domain_set_value((Domain *)ma, "10");
    StructDomain *s = domain_struct_domain_new("Point");
    s->mode = API_MODE_API;
    MemberDomain *mx = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)ma);
    domain_domain_add_child((Domain *)mod, (Domain *)s);

    const char *dir = "/tmp/test_gen_modh1";
    make_tmp_dir(dir);
    generator_generate_mod_h((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modh1/mod.h");
    ASSERT_NOT_NULL(c, "生成 mod.h");
    ASSERT_CONTAINS(c, "#ifndef MOD_H", "ifndef guard");
    ASSERT_CONTAINS(c, "#define MOD_H", "define guard");
    ASSERT_CONTAINS(c, "#include <stdio.h>", "stdio");
    ASSERT_CONTAINS(c, "#include <stdlib.h>", "stdlib");
    ASSERT_CONTAINS(c, "#include <string.h>", "string");
    ASSERT_CONTAINS(c, "#define MAX 10", "API 宏");
    ASSERT_CONTAINS(c, "typedef struct Point {", "API struct");
    ASSERT_CONTAINS(c, "typedef struct Extern Extern;", "前向声明");
    ASSERT_CONTAINS(c, "int mod_init(struct Extern * cfg);", "API 函数声明");
    ASSERT_CONTAINS(c, "#endif /* MOD_H */", "endif guard");
    ASSERT_NOT_CONTAINS(c, "_WIN32", "NORMAL 无 dllexport");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modh1");
    ENDCASE;

    CASE("generate_mod_h: DL 模式输出 dllexport");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_DL;
    FunctionDomain *fa = domain_function_domain_new("open", "int");
    fa->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    const char *dir = "/tmp/test_gen_modh2";
    make_tmp_dir(dir);
    generator_generate_mod_h((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_modh2/lib.h");
    ASSERT_CONTAINS(c, "#ifndef LIB_H", "guard");
    ASSERT_CONTAINS(c, "#ifdef _WIN32", "DL dllexport");
    ASSERT_CONTAINS(c, "lib_API int lib_open();", "DL API 函数");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_modh2");
    ENDCASE;
}

/* ================================================================== */
/* 28. generator_cmake_write_prebuilt_lib (static)                    */
/* ================================================================== */
TEST_SUITE(test_gen_cmake_write_prebuilt_lib) {
    CASE("write_prebuilt_lib: STATIC");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "libfoo");
    mod->mode = MOD_MODE_STATIC;
    domain_domain_set_value((Domain *)mod, "lib/libfoo.a");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cmake_write_prebuilt_lib((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode: static", "static 模式标注");
    ASSERT_CONTAINS(c, "add_library(libfoo STATIC IMPORTED GLOBAL)", "STATIC IMPORTED");
    ASSERT_CONTAINS(c, "IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/libfoo.a", "库路径");
    ASSERT_CONTAINS(c, "预编译静态库", "静态库注释");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_prebuilt_lib: DYNAMIC");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "libbar");
    mod->mode = MOD_MODE_DYNAMIC;
    domain_domain_set_value((Domain *)mod, "lib/libbar.so");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cmake_write_prebuilt_lib((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode: dynamic", "dynamic 模式标注");
    ASSERT_CONTAINS(c, "add_library(libbar SHARED IMPORTED GLOBAL)", "SHARED IMPORTED");
    ASSERT_CONTAINS(c, "lib/libbar.so", "库路径");
    ASSERT_CONTAINS(c, "预编译动态库", "动态库注释");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_prebuilt_lib: 无 value 路径为空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_STATIC;
    FILE *f = open_tmp();
    generator_cmake_write_prebuilt_lib((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/\n", "无 value 空路径");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 29. generator_cmake_write_submodule_sources (static)               */
/* ================================================================== */
TEST_SUITE(test_gen_cmake_write_submodule_sources) {
    CASE("write_submodule_sources: SRC 子模块输出 add_subdirectory");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)sub);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cmake_write_submodule_sources((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "# Sub-modules", "子模块段标题");
    ASSERT_CONTAINS(c, "add_subdirectory(sub)", "add_subdirectory");
    ASSERT_CONTAINS(c, "list(APPEND mod_SOURCES ${sub_SOURCES})", "APPEND 源");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_submodule_sources: 跳过 EXTERNAL/STATIC/DYNAMIC");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *ext = domain_module_domain_new("ext");
    ext->mode = MOD_MODE_EXTERNAL;
    ModuleDomain *st = domain_module_domain_new("st");
    st->mode = MOD_MODE_STATIC;
    ModuleDomain *dy = domain_module_domain_new("dy");
    dy->mode = MOD_MODE_DYNAMIC;
    ModuleDomain *src = domain_module_domain_new("src");
    domain_domain_add_child((Domain *)mod, (Domain *)ext);
    domain_domain_add_child((Domain *)mod, (Domain *)st);
    domain_domain_add_child((Domain *)mod, (Domain *)dy);
    domain_domain_add_child((Domain *)mod, (Domain *)src);
    FILE *f = open_tmp();
    generator_cmake_write_submodule_sources((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "add_subdirectory(src)", "仅 SRC 子模块");
    ASSERT_NOT_CONTAINS(c, "add_subdirectory(ext)", "跳过 EXTERNAL");
    ASSERT_NOT_CONTAINS(c, "add_subdirectory(st)", "跳过 STATIC");
    ASSERT_NOT_CONTAINS(c, "add_subdirectory(dy)", "跳过 DYNAMIC");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_submodule_sources: 无 SRC 子模块输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FILE *f = open_tmp();
    generator_cmake_write_submodule_sources((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无子模块输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 30. generator_generate_mod_cmake (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_generate_mod_cmake) {
    CASE("generate_mod_cmake: NORMAL - PARENT_SCOPE");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    mod->compiler = COMPILER_NORMAL;
    const char *dir = "/tmp/test_gen_cmk1";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk1/CMakeLists.txt");
    ASSERT_CONTAINS(c, "compiler: normal", "头注释 normal");
    ASSERT_CONTAINS(c, "set(mod_SOURCES", "SOURCES 变量");
    ASSERT_CONTAINS(c, "${CMAKE_CURRENT_SOURCE_DIR}/mod.c", "mod.c 源");
    ASSERT_CONTAINS(c, "set(mod_SOURCES ${mod_SOURCES} PARENT_SCOPE)", "PARENT_SCOPE");
    ASSERT_NOT_CONTAINS(c, "add_executable", "NORMAL 无 add_executable");
    ASSERT_NOT_CONTAINS(c, "add_library", "NORMAL 无 add_library");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk1");
    ENDCASE;

    CASE("generate_mod_cmake: EXE - add_executable + main.c");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "app");
    mod->compiler = COMPILER_EXE;
    const char *dir = "/tmp/test_gen_cmk2";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk2/CMakeLists.txt");
    ASSERT_CONTAINS(c, "compiler: exe", "头注释 exe");
    ASSERT_CONTAINS(c, "${CMAKE_CURRENT_SOURCE_DIR}/main.c", "main.c 源");
    ASSERT_CONTAINS(c, "add_executable(app ${app_SOURCES})", "add_executable");
    ASSERT_CONTAINS(c, "target_include_directories(app PRIVATE", "include dirs PRIVATE");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk2");
    ENDCASE;

    CASE("generate_mod_cmake: SL - add_library STATIC");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_SL;
    const char *dir = "/tmp/test_gen_cmk3";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk3/CMakeLists.txt");
    ASSERT_CONTAINS(c, "compiler: sl", "头注释 sl");
    ASSERT_CONTAINS(c, "add_library(lib STATIC ${lib_SOURCES})", "STATIC 库");
    ASSERT_CONTAINS(c, "target_include_directories(lib PUBLIC", "PUBLIC include");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk3");
    ENDCASE;

    CASE("generate_mod_cmake: DL - add_library SHARED + EXPORTS");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->compiler = COMPILER_DL;
    const char *dir = "/tmp/test_gen_cmk4";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk4/CMakeLists.txt");
    ASSERT_CONTAINS(c, "compiler: dl", "头注释 dl");
    ASSERT_CONTAINS(c, "add_library(lib SHARED ${lib_SOURCES})", "SHARED 库");
    ASSERT_CONTAINS(c, "target_compile_definitions(lib PRIVATE lib_EXPORTS)", "EXPORTS 定义");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk4");
    ENDCASE;

    CASE("generate_mod_cmake: STATIC 预编译库走 prebuilt 分支");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "libfoo");
    mod->mode = MOD_MODE_STATIC;
    domain_domain_set_value((Domain *)mod, "lib/libfoo.a");
    const char *dir = "/tmp/test_gen_cmk5";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk5/CMakeLists.txt");
    ASSERT_CONTAINS(c, "mode: static", "prebuilt static");
    ASSERT_CONTAINS(c, "add_library(libfoo STATIC IMPORTED GLOBAL)", "IMPORTED");
    ASSERT_NOT_CONTAINS(c, "compiler:", "prebuilt 不输出 compiler 头");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk5");
    ENDCASE;

    CASE("generate_mod_cmake: DYNAMIC 预编译库走 prebuilt 分支");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "libbar");
    mod->mode = MOD_MODE_DYNAMIC;
    domain_domain_set_value((Domain *)mod, "lib/libbar.so");
    const char *dir = "/tmp/test_gen_cmk6";
    make_tmp_dir(dir);
    generator_generate_mod_cmake((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_cmk6/CMakeLists.txt");
    ASSERT_CONTAINS(c, "mode: dynamic", "prebuilt dynamic");
    ASSERT_CONTAINS(c, "add_library(libbar SHARED IMPORTED GLOBAL)", "IMPORTED SHARED");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_cmk6");
    ENDCASE;
}

/* ================================================================== */
/* 31. generator_write_mod_mode (static)                              */
/* ================================================================== */
TEST_SUITE(test_gen_write_mod_mode) {
    CASE("write_mod_mode: 四种 mode");
    Project *p = domain_project_new("p");
    ModuleDomain *m = make_mod_in_proj(p, "m");

    m->mode = MOD_MODE_SRC; {
        FILE *f = open_tmp(); generator_write_mod_mode(m, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "mode src", "SRC"); free(c);
    }
    m->mode = MOD_MODE_STATIC; {
        FILE *f = open_tmp(); generator_write_mod_mode(m, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "mode static", "STATIC"); free(c);
    }
    m->mode = MOD_MODE_DYNAMIC; {
        FILE *f = open_tmp(); generator_write_mod_mode(m, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "mode dynamic", "DYNAMIC"); free(c);
    }
    m->mode = MOD_MODE_EXTERNAL; {
        FILE *f = open_tmp(); generator_write_mod_mode(m, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "mode external", "EXTERNAL"); free(c);
    }
    domain_project_free(p);
    ENDCASE;

    CASE("write_mod_mode: 非法 mode 不输出");
    Project *p = domain_project_new("p");
    ModuleDomain *m = make_mod_in_proj(p, "m");
    m->mode = (ModMode)999;
    FILE *f = open_tmp();
    generator_write_mod_mode(m, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "非法 mode 不输出");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 32. generator_cboot_write_header (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_header) {
    CASE("write_header: 基本 + cmode 始终输出");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    mod->mode = MOD_MODE_SRC;
    mod->compiler = COMPILER_NORMAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_header((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "# CBoot minimal file for module mod", "模块头注释");
    ASSERT_CONTAINS(c, "mode src", "mode src");
    ASSERT_CONTAINS(c, "cmode normal", "cmode 始终输出");
    ASSERT_NOT_CONTAINS(c, "value ", "SRC 不输出 value");
    ASSERT_NOT_CONTAINS(c, "code <<EOF", "无 code 不输出 code 块");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_header: 模块注释");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    domain_domain_set_comment((Domain *)mod, "模块说明");
    FILE *f = open_tmp();
    generator_cboot_write_header((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "cmt \"模块说明\"", "cmt 注释");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_header: STATIC 输出 value");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_STATIC;
    domain_domain_set_value((Domain *)mod, "lib/libfoo.a");
    mod->compiler = COMPILER_NORMAL;
    FILE *f = open_tmp();
    generator_cboot_write_header((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode static", "mode static");
    ASSERT_CONTAINS(c, "value \"lib/libfoo.a\"", "value 路径");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_header: DYNAMIC 输出 value");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_DYNAMIC;
    domain_domain_set_value((Domain *)mod, "lib/lib.so");
    FILE *f = open_tmp();
    generator_cboot_write_header((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode dynamic", "mode dynamic");
    ASSERT_CONTAINS(c, "value \"lib/lib.so\"", "value 路径");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_header: SRC + code 输出 code 块");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    mod->mode = MOD_MODE_SRC;
    domain_domain_set_code((Domain *)mod, "int f(void) { return 0; }");
    FILE *f = open_tmp();
    generator_cboot_write_header((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "code <<EOF", "code 块开始");
    ASSERT_CONTAINS(c, "int f(void) { return 0; }", "code 内容");
    ASSERT_CONTAINS(c, "EOF", "code 块结束");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_header: 各 compiler 模式 cmode");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "m");
    mod->mode = MOD_MODE_SRC;

    mod->compiler = COMPILER_EXE; {
        FILE *f = open_tmp(); generator_cboot_write_header((Domain *)mod, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "cmode exe", "exe"); free(c);
    }
    mod->compiler = COMPILER_SL; {
        FILE *f = open_tmp(); generator_cboot_write_header((Domain *)mod, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "cmode sl", "sl"); free(c);
    }
    mod->compiler = COMPILER_DL; {
        FILE *f = open_tmp(); generator_cboot_write_header((Domain *)mod, f);
        char *c = close_and_read(f); ASSERT_CONTAINS(c, "cmode dl", "dl"); free(c);
    }
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 33. generator_cboot_write_members (static)                         */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_members) {
    CASE("write_members: 成员 + 注释");
    StructDomain *s = domain_struct_domain_new("S");
    MemberDomain *m1 = domain_member_domain_new("x", "int");
    MemberDomain *m2 = domain_member_domain_new("name", "char*");
    domain_domain_set_comment((Domain *)m2, "名称");
    domain_domain_add_child((Domain *)s, (Domain *)m1);
    domain_domain_add_child((Domain *)s, (Domain *)m2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_members((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mem x int", "成员 x");
    ASSERT_CONTAINS(c, "mem name char*", "成员 name");
    ASSERT_CONTAINS(c, "cmt \"名称\"", "成员注释");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;
}

/* ================================================================== */
/* 34. generator_cboot_write_function (static)                        */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_function) {
    CASE("write_function: API + call + value + 参数");
    FunctionDomain *fn = domain_function_domain_new("init", "int");
    fn->mode = API_MODE_API;
    domain_domain_set_call((Domain *)fn, "__stdcall");
    domain_domain_set_comment((Domain *)fn, "初始化");
    domain_domain_set_value((Domain *)fn, "setup");
    MemberDomain *p1 = domain_member_domain_new("cfg", "void*");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_function((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void init int", "函数声明行");
    ASSERT_CONTAINS(c, "cd init", "cd 进入");
    ASSERT_CONTAINS(c, "mode api", "API 模式");
    ASSERT_CONTAINS(c, "call __stdcall", "call 约定");
    ASSERT_CONTAINS(c, "cmt \"初始化\"", "注释");
    ASSERT_CONTAINS(c, "value \"setup\"", "value");
    ASSERT_CONTAINS(c, "mem cfg void*", "参数成员");
    ASSERT_CONTAINS(c, "cd ..", "cd 退出");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_function: NORMAL 无 mode api");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    fn->mode = API_MODE_NORMAL;
    FILE *f = open_tmp();
    generator_cboot_write_function((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "mode api", "NORMAL 无 mode api");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 35. generator_cboot_write_struct (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_struct) {
    CASE("write_struct: API + 成员");
    StructDomain *s = domain_struct_domain_new("Point");
    s->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)s, "点");
    MemberDomain *mx = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_struct((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "struct Point", "struct 声明");
    ASSERT_CONTAINS(c, "cd Point", "cd 进入");
    ASSERT_CONTAINS(c, "mode api", "API 模式");
    ASSERT_CONTAINS(c, "cmt \"点\"", "注释");
    ASSERT_CONTAINS(c, "mem x int", "成员");
    ASSERT_CONTAINS(c, "cd ..", "cd 退出");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;

    CASE("write_struct: NORMAL 无 mode api");
    StructDomain *s = domain_struct_domain_new("S");
    s->mode = API_MODE_NORMAL;
    FILE *f = open_tmp();
    generator_cboot_write_struct((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "mode api", "NORMAL 无 mode api");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;
}

/* ================================================================== */
/* 36. generator_cboot_write_type (static)                            */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_type) {
    CASE("write_type: RENAME");
    TypeDomain *t = domain_type_domain_new("MyInt");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)t, "long");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_type((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "type MyInt", "type 声明");
    ASSERT_CONTAINS(c, "mode rename", "rename 模式");
    ASSERT_CONTAINS(c, "value \"long\"", "value");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("write_type: API_RENAME");
    TypeDomain *t = domain_type_domain_new("H");
    t->mode = TYPE_MODE_API_RENAME;
    domain_domain_set_value((Domain *)t, "void*");
    FILE *f = open_tmp();
    generator_cboot_write_type((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode api rename", "api rename 模式");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("write_type: API_STRUCT");
    TypeDomain *t = domain_type_domain_new("Op");
    t->mode = TYPE_MODE_API_STRUCT;
    FILE *f = open_tmp();
    generator_cboot_write_type((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "mode api struct", "api struct 模式");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("write_type: STRUCT(非 API) 不输出 mode");
    TypeDomain *t = domain_type_domain_new("Op");
    t->mode = TYPE_MODE_STRUCT;
    FILE *f = open_tmp();
    generator_cboot_write_type((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "type Op", "type 声明");
    ASSERT_NOT_CONTAINS(c, "mode ", "STRUCT 非 API 无 mode 行");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;
}

/* ================================================================== */
/* 37. generator_cboot_write_macro (static)                           */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_macro) {
    CASE("write_macro: API + value + 注释");
    MacroDomain *m = domain_macro_domain_new("MAX");
    m->mode = API_MODE_API;
    domain_domain_set_value((Domain *)m, "256");
    domain_domain_set_comment((Domain *)m, "上限");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_macro((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "def MAX", "def 声明");
    ASSERT_CONTAINS(c, "cd MAX", "cd 进入");
    ASSERT_CONTAINS(c, "mode api", "API 模式");
    ASSERT_CONTAINS(c, "cmt \"上限\"", "注释");
    ASSERT_CONTAINS(c, "value \"256\"", "value");
    ASSERT_CONTAINS(c, "cd ..", "cd 退出");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("write_macro: NORMAL 无 mode api");
    MacroDomain *m = domain_macro_domain_new("DBG");
    m->mode = API_MODE_NORMAL;
    FILE *f = open_tmp();
    generator_cboot_write_macro((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "mode api", "NORMAL 无 mode api");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;
}

/* ================================================================== */
/* 38. generator_cboot_write_variable (static)                        */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_variable) {
    CASE("write_variable: STATIC + value + 注释");
    VariableDomain *v = domain_variable_domain_new("count", "int");
    v->mode = VAR_MODE_STATIC;
    domain_domain_set_value((Domain *)v, "0");
    domain_domain_set_comment((Domain *)v, "计数");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_variable((Domain *)v, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "var count int", "var 声明");
    ASSERT_CONTAINS(c, "cd count", "cd 进入");
    ASSERT_CONTAINS(c, "mode static", "static 模式");
    ASSERT_CONTAINS(c, "cmt \"计数\"", "注释");
    ASSERT_CONTAINS(c, "value \"0\"", "value");
    ASSERT_CONTAINS(c, "cd ..", "cd 退出");
    free(c);
    domain_domain_delete((Domain *)v);
    ENDCASE;

    CASE("write_variable: NORMAL 无 mode static");
    VariableDomain *v = domain_variable_domain_new("buf", "char*");
    v->mode = VAR_MODE_NORMAL;
    FILE *f = open_tmp();
    generator_cboot_write_variable((Domain *)v, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "var buf char*", "var 声明");
    ASSERT_NOT_CONTAINS(c, "mode static", "NORMAL 无 mode static");
    free(c);
    domain_domain_delete((Domain *)v);
    ENDCASE;
}

/* ================================================================== */
/* 39. generator_cboot_write_children (static)                        */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_children) {
    CASE("write_children: 分发各类子域 + 跳过 MODULE/MEMBER");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    fn->mode = API_MODE_API;
    StructDomain *s = domain_struct_domain_new("S");
    s->mode = API_MODE_API;
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_RENAME;
    MacroDomain *m = domain_macro_domain_new("M");
    m->mode = API_MODE_API;
    VariableDomain *v = domain_variable_domain_new("count", "int");
    ModuleDomain *sub = domain_module_domain_new("sub");  /* 应被跳过 */
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    domain_domain_add_child((Domain *)mod, (Domain *)m);
    domain_domain_add_child((Domain *)mod, (Domain *)v);
    domain_domain_add_child((Domain *)mod, (Domain *)sub);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_children((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void f int", "function");
    ASSERT_CONTAINS(c, "struct S", "struct");
    ASSERT_CONTAINS(c, "type T", "type");
    ASSERT_CONTAINS(c, "def M", "macro");
    ASSERT_CONTAINS(c, "var count int", "variable");
    ASSERT_NOT_CONTAINS(c, "mod sub", "MODULE 子节点被跳过");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 40. generator_cboot_write_submodule_refs (static)                  */
/* ================================================================== */
TEST_SUITE(test_gen_cboot_write_submodule_refs) {
    CASE("write_submodule_refs: EXTERNAL 输出 im，其它输出 .cboot");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *ext = domain_module_domain_new("ext");
    ext->mode = MOD_MODE_EXTERNAL;
    ModuleDomain *src = domain_module_domain_new("src");
    ModuleDomain *st = domain_module_domain_new("st");
    st->mode = MOD_MODE_STATIC;
    domain_domain_add_child((Domain *)mod, (Domain *)ext);
    domain_domain_add_child((Domain *)mod, (Domain *)src);
    domain_domain_add_child((Domain *)mod, (Domain *)st);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    generator_cboot_write_submodule_refs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "# 子模块引用", "段标题");
    ASSERT_CONTAINS(c, "im ext", "EXTERNAL → im");
    ASSERT_CONTAINS(c, "src/.cboot", "SRC → .cboot 引用");
    ASSERT_CONTAINS(c, "st/.cboot", "STATIC → .cboot 引用");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_submodule_refs: 无子模块输出空");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FILE *f = open_tmp();
    generator_cboot_write_submodule_refs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_TRUE(c != NULL && c[0] == '\0', "无子模块输出为空");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 41. generator_generate_mod_cboot (static)                          */
/* ================================================================== */
TEST_SUITE(test_gen_generate_mod_cboot) {
    CASE("generate_mod_cboot: 完整 .cboot 文件");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    mod->mode = MOD_MODE_SRC;
    mod->compiler = COMPILER_NORMAL;
    domain_domain_set_comment((Domain *)mod, "模块说明");
    FunctionDomain *fn = domain_function_domain_new("init", "int");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)sub);

    const char *dir = "/tmp/test_gen_mcb1";
    make_tmp_dir(dir);
    generator_generate_mod_cboot((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_mcb1/.cboot");
    ASSERT_NOT_NULL(c, "生成 .cboot");
    ASSERT_CONTAINS(c, "# CBoot minimal file for module mod", "模块头");
    ASSERT_CONTAINS(c, "cmt \"模块说明\"", "模块注释");
    ASSERT_CONTAINS(c, "mode src", "mode src");
    ASSERT_CONTAINS(c, "cmode normal", "cmode");
    ASSERT_CONTAINS(c, "void init int", "函数子域");
    ASSERT_CONTAINS(c, "sub/.cboot", "子模块引用");
    ASSERT_CONTAINS(c, "# End of .cboot for mod", "结尾");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_mcb1");
    ENDCASE;

    CASE("generate_mod_cboot: EXTERNAL 子模块输出 im");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *ext = domain_module_domain_new("ext");
    ext->mode = MOD_MODE_EXTERNAL;
    domain_domain_add_child((Domain *)mod, (Domain *)ext);
    const char *dir = "/tmp/test_gen_mcb2";
    make_tmp_dir(dir);
    generator_generate_mod_cboot((Domain *)mod, dir);
    char *c = read_file("/tmp/test_gen_mcb2/.cboot");
    /* EXTERNAL 子模块在两处输出 im：header 循环 + submodule_refs */
    ASSERT_CONTAINS(c, "im ext", "EXTERNAL → im");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_mcb2");
    ENDCASE;
}

/* ================================================================== */
/* 42. generator_find_exe_module (static)                             */
/* ================================================================== */
TEST_SUITE(test_gen_find_exe_module) {
    CASE("find_exe_module: 找到 exe 模块");
    Project *p = domain_project_new("p");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    Domain *found = generator_find_exe_module(p);
    ASSERT_NOT_NULL(found, "应找到 exe 模块");
    ASSERT_EQ_STR(found->name, "app", "找到 app");
    (void)m1; (void)m2;
    domain_project_free(p);
    ENDCASE;

    CASE("find_exe_module: 无 exe 模块返回 NULL");
    Project *p = domain_project_new("p");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    Domain *found = generator_find_exe_module(p);
    ASSERT_NULL(found, "无 exe 返回 NULL");
    (void)m1; (void)m2;
    domain_project_free(p);
    ENDCASE;

    CASE("find_exe_module: 跳过非 MODULE 子节点");
    Project *p = domain_project_new("p");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    fn->mode = API_MODE_NORMAL;  /* 防止误判 */
    domain_domain_add_child(p->root, (Domain *)fn);
    Domain *found = generator_find_exe_module(p);
    ASSERT_NULL(found, "非 MODULE 不误判");
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 43. top_cmake_* 辅助函数 (static)                                  */
/* ================================================================== */
TEST_SUITE(test_gen_top_cmake_helpers) {
    CASE("top_cmake_add_subdirs: 跳过 exe/external/skip");
    Project *p = domain_project_new("p");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    ModuleDomain *ext = make_mod_in_proj(p, "ext");
    ext->mode = MOD_MODE_EXTERNAL;
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    top_cmake_add_subdirs(f, p, (Domain *)app);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "add_subdirectory(m1)", "m1");
    ASSERT_CONTAINS(c, "add_subdirectory(m2)", "m2");
    ASSERT_NOT_CONTAINS(c, "add_subdirectory(app)", "跳过 exe(skip)");
    ASSERT_NOT_CONTAINS(c, "add_subdirectory(ext)", "跳过 external");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("top_cmake_emit_exe_sources: 仅 NORMAL+SRC 加入 exe");
    Project *p = domain_project_new("p");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");  /* NORMAL SRC */
    ModuleDomain *sl = make_mod_in_proj(p, "sl");
    sl->compiler = COMPILER_SL;
    ModuleDomain *st = make_mod_in_proj(p, "st");
    st->mode = MOD_MODE_STATIC;
    ModuleDomain *ext = make_mod_in_proj(p, "ext");
    ext->mode = MOD_MODE_EXTERNAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    top_cmake_emit_exe_sources(f, p, (Domain *)app);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "target_sources(app PRIVATE ${n1_SOURCES})", "NORMAL n1 加入");
    ASSERT_NOT_CONTAINS(c, "sl_SOURCES", "SL 不加入 sources");
    ASSERT_NOT_CONTAINS(c, "st_SOURCES", "STATIC 不加入 sources");
    ASSERT_NOT_CONTAINS(c, "ext_SOURCES", "EXTERNAL 不加入");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("top_cmake_emit_exe_libs: SL/DL/STATIC/DYNAMIC 链接");
    Project *p = domain_project_new("p");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    ModuleDomain *sl = make_mod_in_proj(p, "sl");
    sl->compiler = COMPILER_SL;
    ModuleDomain *dl = make_mod_in_proj(p, "dl");
    dl->compiler = COMPILER_DL;
    ModuleDomain *st = make_mod_in_proj(p, "st");
    st->mode = MOD_MODE_STATIC;
    ModuleDomain *dy = make_mod_in_proj(p, "dy");
    dy->mode = MOD_MODE_DYNAMIC;
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");  /* NORMAL 不链接 */
    ModuleDomain *ext = make_mod_in_proj(p, "ext");
    ext->mode = MOD_MODE_EXTERNAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    top_cmake_emit_exe_libs(f, p, (Domain *)app);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "target_link_libraries(app PRIVATE sl)", "SL 链接");
    ASSERT_CONTAINS(c, "target_link_libraries(app PRIVATE dl)", "DL 链接");
    ASSERT_CONTAINS(c, "target_link_libraries(app PRIVATE st)", "STATIC 链接");
    ASSERT_CONTAINS(c, "target_link_libraries(app PRIVATE dy)", "DYNAMIC 链接");
    ASSERT_NOT_CONTAINS(c, "PRIVATE n1)", "NORMAL 不链接");
    ASSERT_NOT_CONTAINS(c, "PRIVATE ext)", "EXTERNAL 不链接");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("top_cmake_collect_all_sources: NORMAL+SRC 收集");
    Project *p = domain_project_new("p");
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");
    ModuleDomain *sl = make_mod_in_proj(p, "sl");
    sl->compiler = COMPILER_SL;
    ModuleDomain *st = make_mod_in_proj(p, "st");
    st->mode = MOD_MODE_STATIC;
    ModuleDomain *ext = make_mod_in_proj(p, "ext");
    ext->mode = MOD_MODE_EXTERNAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    top_cmake_collect_all_sources(f, p);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "set(ALL_SOURCES main.c)", "main.c 基础源");
    ASSERT_CONTAINS(c, "list(APPEND ALL_SOURCES ${n1_SOURCES})", "n1 收集");
    ASSERT_NOT_CONTAINS(c, "sl_SOURCES", "SL 不收集");
    ASSERT_NOT_CONTAINS(c, "st_SOURCES", "STATIC 不收集");
    ASSERT_NOT_CONTAINS(c, "ext_SOURCES", "EXTERNAL 不收集");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("top_cmake_link_prebuilt: STATIC/DYNAMIC 链接到目标");
    Project *p = domain_project_new("p");
    ModuleDomain *st = make_mod_in_proj(p, "st");
    st->mode = MOD_MODE_STATIC;
    ModuleDomain *dy = make_mod_in_proj(p, "dy");
    dy->mode = MOD_MODE_DYNAMIC;
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    top_cmake_link_prebuilt(f, p, "mytarget");
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "target_link_libraries(mytarget PRIVATE st)", "STATIC 链接");
    ASSERT_CONTAINS(c, "target_link_libraries(mytarget PRIVATE dy)", "DYNAMIC 链接");
    ASSERT_NOT_CONTAINS(c, "PRIVATE n1)", "NORMAL 不链接");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 44. generator_generate_top_cmake (static, 写 CWD)                  */
/* ================================================================== */
TEST_SUITE(test_gen_generate_top_cmake) {
    CASE("generate_top_cmake: 无 exe 模块 → add_executable(proj)");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    const char *dir = "/tmp/test_gen_topcmk1";
    enter_tmp(dir);
    generator_generate_top_cmake(p);
    char *c = read_file("CMakeLists.txt");
    ASSERT_NOT_NULL(c, "生成 CMakeLists.txt");
    ASSERT_CONTAINS(c, "cmake_minimum_required(VERSION 3.13)", "cmake 版本");
    ASSERT_CONTAINS(c, "project(myproj C)", "project");
    ASSERT_CONTAINS(c, "set(CMAKE_C_STANDARD 11)", "C 标准");
    ASSERT_CONTAINS(c, "set(CMAKE_C_STANDARD_REQUIRED ON)", "标准必须");
    ASSERT_CONTAINS(c, "add_subdirectory(m1)", "子目录 m1");
    ASSERT_CONTAINS(c, "set(ALL_SOURCES main.c)", "ALL_SOURCES");
    ASSERT_CONTAINS(c, "add_executable(myproj ${ALL_SOURCES})", "可执行目标");
    ASSERT_CONTAINS(c, "target_include_directories(myproj PRIVATE ${CMAKE_SOURCE_DIR})", "include dirs");
    ASSERT_CONTAINS(c, "# End of CMakeLists.txt", "结尾");
    free(c);
    (void)m1;
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_top_cmake: 有 exe 模块 → 不生成 ALL_SOURCES");
    Project *p = domain_project_new("myproj");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");
    ModuleDomain *sl = make_mod_in_proj(p, "sl");
    sl->compiler = COMPILER_SL;
    const char *dir = "/tmp/test_gen_topcmk2";
    enter_tmp(dir);
    generator_generate_top_cmake(p);
    char *c = read_file("CMakeLists.txt");
    ASSERT_CONTAINS(c, "# Executable module", "exe 段标题");
    ASSERT_CONTAINS(c, "add_subdirectory(app)", "exe 子目录");
    ASSERT_CONTAINS(c, "add_subdirectory(n1)", "n1 子目录");
    ASSERT_CONTAINS(c, "target_sources(app PRIVATE ${n1_SOURCES})", "exe 收集 n1 源");
    ASSERT_CONTAINS(c, "target_link_libraries(app PRIVATE sl)", "exe 链接 sl");
    ASSERT_CONTAINS(c, "target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR})", "exe include");
    ASSERT_NOT_CONTAINS(c, "add_executable(myproj", "无 exe 时不生成项目级 exe");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 45. generator_generate_top_main (static, 写 CWD)                   */
/* ================================================================== */
TEST_SUITE(test_gen_generate_top_main) {
    CASE("generate_top_main: 无 exe 模块 → 生成 main.c");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    domain_domain_set_comment(p->root, "项目说明");
    const char *dir = "/tmp/test_gen_topmain1";
    enter_tmp(dir);
    generator_generate_top_main(p);
    ASSERT_TRUE(utils_file_exists("main.c"), "生成 main.c");
    char *c = read_file("main.c");
    ASSERT_CONTAINS(c, "/* main.c - CBoot generated entry point */", "头注释");
    ASSERT_CONTAINS(c, "#include <stdio.h>", "stdio");
    ASSERT_CONTAINS(c, "#include \"m1/m1.h\"", "m1 头");
    ASSERT_CONTAINS(c, "#include \"m2/m2.h\"", "m2 头");
    ASSERT_CONTAINS(c, "int main(int argc, char **argv) {", "main 签名");
    ASSERT_CONTAINS(c, "(void)argc;", "argc 抑制");
    ASSERT_CONTAINS(c, "(void)argv;", "argv 抑制");
    ASSERT_CONTAINS(c, "/* 项目说明 */", "根注释");
    ASSERT_CONTAINS(c, "printf(\"Hello from myproj!\\n\");", "printf");
    ASSERT_CONTAINS(c, "return 0;", "return 0");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_top_main: 有 exe 模块 → 不生成 main.c");
    Project *p = domain_project_new("myproj");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    const char *dir = "/tmp/test_gen_topmain2";
    enter_tmp(dir);
    generator_generate_top_main(p);
    ASSERT_FALSE(utils_file_exists("main.c"), "有 exe 不生成顶层 main.c");
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 46. generator_generate_project_cboot (static, 写 CWD)              */
/* ================================================================== */
TEST_SUITE(test_gen_generate_project_cboot) {
    CASE("generate_project_cboot: 基本结构 + 模块引用 + gen");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    const char *dir = "/tmp/test_gen_projcb1";
    enter_tmp(dir);
    generator_generate_project_cboot(p);
    ASSERT_TRUE(utils_file_exists(".cboot"), "生成 .cboot");
    char *c = read_file(".cboot");
    ASSERT_NOT_NULL(c, "读取 .cboot");
    ASSERT_CONTAINS(c, "# CBoot project file for myproj", "项目头");
    ASSERT_CONTAINS(c, "project myproj", "project 声明");
    ASSERT_CONTAINS(c, "# 顶级模块引用", "模块引用段");
    ASSERT_CONTAINS(c, "m1/.cboot", "m1 引用");
    ASSERT_CONTAINS(c, "m2/.cboot", "m2 引用");
    ASSERT_CONTAINS(c, "\ngen\n", "gen 指令");
    ASSERT_NOT_CONTAINS(c, "im-dep", "无依赖");
    ASSERT_NOT_CONTAINS(c, "in-import", "无导入");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_project_cboot: 有 im 依赖");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    domain_project_add_dependency(p, "/a", "/b", "b.cboot");
    domain_project_add_dependency(p, "/c", "/d", NULL);
    const char *dir = "/tmp/test_gen_projcb2";
    enter_tmp(dir);
    generator_generate_project_cboot(p);
    char *c = read_file(".cboot");
    ASSERT_CONTAINS(c, "# im 依赖链记录:", "im 段标题");
    ASSERT_CONTAINS(c, "# im-dep: /a -> /b (b.cboot)", "依赖(含 cboot)");
    ASSERT_CONTAINS(c, "# im-dep: /c -> /d", "依赖(无 cboot)");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_project_cboot: 有 in 导入");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    proj_add_import(p, "libfoo.cboot");
    proj_add_import(p, "libbar.cboot");
    const char *dir = "/tmp/test_gen_projcb3";
    enter_tmp(dir);
    generator_generate_project_cboot(p);
    char *c = read_file(".cboot");
    ASSERT_CONTAINS(c, "# in 完整项目导入记录:", "in 段标题");
    ASSERT_CONTAINS(c, "# in-import: libfoo.cboot", "in 导入 1");
    ASSERT_CONTAINS(c, "# in-import: libbar.cboot", "in 导入 2");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 47. generator_generate_cboot_only (public, 写 CWD)                 */
/* ================================================================== */
TEST_SUITE(test_gen_generate_cboot_only) {
    CASE("generate_cboot_only: NULL 返回 -1");
    ASSERT_EQ_INT(generator_generate_cboot_only(NULL), -1, "NULL 返回 -1");
    ENDCASE;

    CASE("generate_cboot_only: root NULL 返回 -1");
    Project *p = domain_project_new("p");
    Domain *r = p->root;
    p->root = NULL;
    ASSERT_EQ_INT(generator_generate_cboot_only(p), -1, "root NULL 返回 -1");
    p->root = r;
    domain_project_free(p);
    ENDCASE;

    CASE("generate_cboot_only: 生成各模块 .cboot + 项目 .cboot");
    Project *p = domain_project_new("p");
    ModuleDomain *a = make_mod_in_proj(p, "a");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)a, (Domain *)sub);
    ModuleDomain *b = make_mod_in_proj(p, "b");

    const char *dir = "/tmp/test_gen_cbo1";
    enter_tmp(dir);
    ASSERT_EQ_INT(generator_generate_cboot_only(p), 0, "应返回 0");
    ASSERT_TRUE(utils_file_exists(".cboot"), "项目 .cboot");
    ASSERT_TRUE(utils_file_exists("a/.cboot"), "a 模块 .cboot");
    ASSERT_TRUE(utils_file_exists("a/sub/.cboot"), "a/sub 模块 .cboot");
    ASSERT_TRUE(utils_file_exists("b/.cboot"), "b 模块 .cboot");
    /* 仅生成 .cboot，不应生成 .c/.h/CMake */
    ASSERT_FALSE(utils_file_exists("a/a.c"), "不生成 a.c");
    ASSERT_FALSE(utils_file_exists("a/a.h"), "不生成 a.h");
    ASSERT_FALSE(utils_file_exists("a/CMakeLists.txt"), "不生成 CMake");
    char *c = read_file("a/.cboot");
    ASSERT_CONTAINS(c, "# CBoot minimal file for module a", "a .cboot 头");
    free(c);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_cboot_only: 跳过 EXTERNAL 子模块");
    Project *p = domain_project_new("p");
    ModuleDomain *a = make_mod_in_proj(p, "a");
    ModuleDomain *ext = domain_module_domain_new("ext");
    ext->mode = MOD_MODE_EXTERNAL;
    ModuleDomain *src = domain_module_domain_new("src");
    domain_domain_add_child((Domain *)a, (Domain *)ext);
    domain_domain_add_child((Domain *)a, (Domain *)src);

    const char *dir = "/tmp/test_gen_cbo2";
    enter_tmp(dir);
    generator_generate_cboot_only(p);
    ASSERT_TRUE(utils_file_exists("a/.cboot"), "a .cboot");
    ASSERT_TRUE(utils_file_exists("a/src/.cboot"), "src 子模块 .cboot");
    ASSERT_FALSE(utils_file_exists("a/ext/.cboot"), "EXTERNAL 子模块被跳过");
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_cboot_only: docgen 不被调用");
    Project *p = domain_project_new("p");
    ModuleDomain *a = make_mod_in_proj(p, "a");
    const char *dir = "/tmp/test_gen_cbo3";
    enter_tmp(dir);
    g_docgen_call_count = 0;
    generator_generate_cboot_only(p);
    ASSERT_EQ_INT(g_docgen_call_count, 0, "cboot_only 不调用 docgen");
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 48. generator_generate_module (static, 递归 + 各 mode)              */
/* ================================================================== */
TEST_SUITE(test_gen_generate_module) {
    CASE("generate_module: NULL / 非 module 安全返回");
    g_docgen_call_count = 0;
    generator_generate_module(NULL, "/tmp/test_gen_gm_null");
    ASSERT_EQ_INT(g_docgen_call_count, 0, "NULL 不调 docgen");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    generator_generate_module((Domain *)fn, "/tmp/test_gen_gm_nonmod");
    ASSERT_EQ_INT(g_docgen_call_count, 0, "非 module 不调 docgen");
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("generate_module: SRC 生成 .c/.h/CMake/.cboot + docgen");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fn);
    const char *dir = "/tmp/test_gen_gm1";
    make_tmp_dir(dir);
    g_docgen_call_count = 0;
    generator_generate_module((Domain *)mod, dir);
    ASSERT_EQ_INT(g_docgen_call_count, 1, "SRC 调一次 docgen");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm1/mod/mod.c"), "mod.c");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm1/mod/mod.h"), "mod.h");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm1/mod/CMakeLists.txt"), "CMake");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm1/mod/.cboot"), ".cboot");
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_gm1");
    ENDCASE;

    CASE("generate_module: STATIC 仅 .h/CMake/.cboot，无 .c");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_STATIC;
    domain_domain_set_value((Domain *)mod, "lib/lib.a");
    const char *dir = "/tmp/test_gen_gm2";
    make_tmp_dir(dir);
    g_docgen_call_count = 0;
    generator_generate_module((Domain *)mod, dir);
    ASSERT_EQ_INT(g_docgen_call_count, 1, "STATIC 调一次 docgen");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm2/lib/lib.h"), "lib.h");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm2/lib/CMakeLists.txt"), "CMake");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm2/lib/.cboot"), ".cboot");
    ASSERT_FALSE(utils_file_exists("/tmp/test_gen_gm2/lib/lib.c"), "无 lib.c");
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_gm2");
    ENDCASE;

    CASE("generate_module: DYNAMIC 仅 .h/CMake/.cboot，无 .c");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "lib");
    mod->mode = MOD_MODE_DYNAMIC;
    domain_domain_set_value((Domain *)mod, "lib/lib.so");
    const char *dir = "/tmp/test_gen_gm3";
    make_tmp_dir(dir);
    g_docgen_call_count = 0;
    generator_generate_module((Domain *)mod, dir);
    ASSERT_EQ_INT(g_docgen_call_count, 1, "DYNAMIC 调一次 docgen");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm3/lib/lib.h"), "lib.h");
    ASSERT_FALSE(utils_file_exists("/tmp/test_gen_gm3/lib/lib.c"), "无 lib.c");
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_gm3");
    ENDCASE;

    CASE("generate_module: EXTERNAL 仅 .h/.cboot，无 .c/CMake");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "ext");
    mod->mode = MOD_MODE_EXTERNAL;
    const char *dir = "/tmp/test_gen_gm4";
    make_tmp_dir(dir);
    g_docgen_call_count = 0;
    generator_generate_module((Domain *)mod, dir);
    ASSERT_EQ_INT(g_docgen_call_count, 1, "EXTERNAL 调一次 docgen");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm4/ext/ext.h"), "ext.h");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm4/ext/.cboot"), ".cboot");
    ASSERT_FALSE(utils_file_exists("/tmp/test_gen_gm4/ext/ext.c"), "无 ext.c");
    ASSERT_FALSE(utils_file_exists("/tmp/test_gen_gm4/ext/CMakeLists.txt"), "无 CMake");
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_gm4");
    ENDCASE;

    CASE("generate_module: 递归子模块");
    Project *p = domain_project_new("p");
    ModuleDomain *mod = make_mod_in_proj(p, "mod");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)sub);
    const char *dir = "/tmp/test_gen_gm5";
    make_tmp_dir(dir);
    g_docgen_call_count = 0;
    generator_generate_module((Domain *)mod, dir);
    ASSERT_EQ_INT(g_docgen_call_count, 2, "父+子调两次 docgen");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm5/mod/mod.c"), "父 mod.c");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm5/mod/sub/sub.c"), "子 sub.c");
    ASSERT_TRUE(utils_file_exists("/tmp/test_gen_gm5/mod/sub/.cboot"), "子 .cboot");
    domain_project_free(p);
    system("rm -rf /tmp/test_gen_gm5");
    ENDCASE;
}

/* ================================================================== */
/* 49. generator_generate_project (public, 写 CWD)                    */
/* ================================================================== */
TEST_SUITE(test_gen_generate_project) {
    CASE("generate_project: NULL 返回 -1");
    ASSERT_EQ_INT(generator_generate_project(NULL), -1, "NULL 返回 -1");
    ENDCASE;

    CASE("generate_project: root NULL 返回 -1");
    Project *p = domain_project_new("p");
    Domain *r = p->root;
    p->root = NULL;
    ASSERT_EQ_INT(generator_generate_project(p), -1, "root NULL 返回 -1");
    p->root = r;
    domain_project_free(p);
    ENDCASE;

    CASE("generate_project: 无 exe 模块完整流程");
    Project *p = domain_project_new("myproj");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)m1, (Domain *)fn);
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");

    const char *dir = "/tmp/test_gen_gp1";
    enter_tmp(dir);
    ASSERT_EQ_INT(generator_generate_project(p), 0, "应返回 0");
    ASSERT_EQ_INT(p->has_generated, 1, "has_generated 置 1");
    /* 模块文件 */
    ASSERT_TRUE(utils_file_exists("m1/m1.c"), "m1.c");
    ASSERT_TRUE(utils_file_exists("m1/m1.h"), "m1.h");
    ASSERT_TRUE(utils_file_exists("m1/CMakeLists.txt"), "m1 CMake");
    ASSERT_TRUE(utils_file_exists("m1/.cboot"), "m1 .cboot");
    ASSERT_TRUE(utils_file_exists("m2/m2.c"), "m2.c");
    /* 顶层文件 */
    ASSERT_TRUE(utils_file_exists("CMakeLists.txt"), "顶层 CMake");
    ASSERT_TRUE(utils_file_exists("main.c"), "顶层 main.c");
    ASSERT_TRUE(utils_file_exists(".cboot"), "项目 .cboot");
    char *cm = read_file("CMakeLists.txt");
    ASSERT_CONTAINS(cm, "add_executable(myproj ${ALL_SOURCES})", "无 exe 项目级 exe");
    free(cm);
    char *mn = read_file("main.c");
    ASSERT_CONTAINS(mn, "Hello from myproj!", "main 内容");
    free(mn);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_project: 有 exe 模块流程");
    Project *p = domain_project_new("myproj");
    ModuleDomain *app = make_mod_in_proj(p, "app");
    app->compiler = COMPILER_EXE;
    FunctionDomain *mf = domain_function_domain_new("main", "int");
    domain_domain_set_code((Domain *)mf, "    return 0;");
    domain_domain_add_child((Domain *)app, (Domain *)mf);
    ModuleDomain *n1 = make_mod_in_proj(p, "n1");

    const char *dir = "/tmp/test_gen_gp2";
    enter_tmp(dir);
    ASSERT_EQ_INT(generator_generate_project(p), 0, "应返回 0");
    ASSERT_EQ_INT(p->has_generated, 1, "has_generated 置 1");
    /* exe 模块用 main.c */
    ASSERT_TRUE(utils_file_exists("app/main.c"), "app/main.c");
    ASSERT_FALSE(utils_file_exists("app/app.c"), "exe 无 app.c");
    /* 顶层不生成 main.c（由 exe 模块提供） */
    ASSERT_FALSE(utils_file_exists("main.c"), "有 exe 无顶层 main.c");
    ASSERT_TRUE(utils_file_exists("CMakeLists.txt"), "顶层 CMake");
    ASSERT_TRUE(utils_file_exists(".cboot"), "项目 .cboot");
    ASSERT_TRUE(utils_file_exists("n1/n1.c"), "n1.c");
    char *cm = read_file("CMakeLists.txt");
    ASSERT_CONTAINS(cm, "add_subdirectory(app)", "exe 子目录");
    ASSERT_NOT_CONTAINS(cm, "add_executable(myproj", "无项目级 exe");
    free(cm);
    /* add_executable(app ...) 写在模块自己的 app/CMakeLists.txt 中 */
    char *cm_app = read_file("app/CMakeLists.txt");
    ASSERT_CONTAINS(cm_app, "add_executable(app ${app_SOURCES})", "exe 目标在 app/CMakeLists.txt");
    free(cm_app);
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;

    CASE("generate_project: docgen 对每个模块调用");
    Project *p = domain_project_new("p");
    ModuleDomain *m1 = make_mod_in_proj(p, "m1");
    ModuleDomain *m2 = make_mod_in_proj(p, "m2");
    const char *dir = "/tmp/test_gen_gp3";
    enter_tmp(dir);
    g_docgen_call_count = 0;
    generator_generate_project(p);
    ASSERT_EQ_INT(g_docgen_call_count, 2, "两个模块各调一次");
    leave_tmp(dir);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 主函数                                                              */
/* ================================================================== */
int main(void) {
    /* 保存真实工作目录，供 chdir 测试返回 */
    getcwd(g_orig_cwd, sizeof(g_orig_cwd));

    /* 按任务要求初始化全局项目 */
    g_proj = domain_project_new("test");
    g_mode = MODE_INTERACTIVE;

    printf("========== generator/generator.c 单元测试 ==========\n");

    RUN_SUITE(test_gen_is_api);
    RUN_SUITE(test_gen_compiler_mode_str);
    RUN_SUITE(test_gen_get_module_prefix);
    RUN_SUITE(test_gen_make_abs_name);
    RUN_SUITE(test_gen_extract_struct_name);
    RUN_SUITE(test_gen_fwd_list_contains);
    RUN_SUITE(test_gen_defined_in_module);
    RUN_SUITE(test_gen_collect_func_types);
    RUN_SUITE(test_gen_fwd_try_add);
    RUN_SUITE(test_gen_write_function_params);
    RUN_SUITE(test_gen_write_func_signature);
    RUN_SUITE(test_gen_write_func_vars);
    RUN_SUITE(test_gen_write_struct_members);
    RUN_SUITE(test_gen_write_c_includes);
    RUN_SUITE(test_gen_write_c_defs);
    RUN_SUITE(test_gen_write_c_types);
    RUN_SUITE(test_gen_write_c_variables);
    RUN_SUITE(test_gen_write_c_functions);
    RUN_SUITE(test_gen_write_h_guard);
    RUN_SUITE(test_gen_write_dllexport_macro);
    RUN_SUITE(test_gen_write_h_api_macros);
    RUN_SUITE(test_gen_write_api_rename_type);
    RUN_SUITE(test_gen_write_h_api_types);
    RUN_SUITE(test_gen_write_h_fwd_decls);
    RUN_SUITE(test_gen_write_h_api_functions);
    RUN_SUITE(test_gen_generate_mod_c);
    RUN_SUITE(test_gen_generate_mod_h);
    RUN_SUITE(test_gen_cmake_write_prebuilt_lib);
    RUN_SUITE(test_gen_cmake_write_submodule_sources);
    RUN_SUITE(test_gen_generate_mod_cmake);
    RUN_SUITE(test_gen_write_mod_mode);
    RUN_SUITE(test_gen_cboot_write_header);
    RUN_SUITE(test_gen_cboot_write_members);
    RUN_SUITE(test_gen_cboot_write_function);
    RUN_SUITE(test_gen_cboot_write_struct);
    RUN_SUITE(test_gen_cboot_write_type);
    RUN_SUITE(test_gen_cboot_write_macro);
    RUN_SUITE(test_gen_cboot_write_variable);
    RUN_SUITE(test_gen_cboot_write_children);
    RUN_SUITE(test_gen_cboot_write_submodule_refs);
    RUN_SUITE(test_gen_generate_mod_cboot);
    RUN_SUITE(test_gen_find_exe_module);
    RUN_SUITE(test_gen_top_cmake_helpers);
    RUN_SUITE(test_gen_generate_top_cmake);
    RUN_SUITE(test_gen_generate_top_main);
    RUN_SUITE(test_gen_generate_project_cboot);
    RUN_SUITE(test_gen_generate_cboot_only);
    RUN_SUITE(test_gen_generate_module);
    RUN_SUITE(test_gen_generate_project);

    /* 清理临时文件 + 恢复工作目录 + 释放全局项目 */
    remove(TMP_FILE);
    chdir(g_orig_cwd);
    domain_project_free(g_proj);
    g_proj = NULL;

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}