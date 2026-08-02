/*
 * test_docgen.c - docgen/docgen.c 单元测试
 *
 * 通过 #include "docgen/docgen.c" 暴露 static 函数进行测试。
 * docgen.c 依赖: utils/utils.c, domain/core/core.c, domain/domain.c,
 *                 typecheck/typecheck.c, docgen/docgen.c
 * docgen.c 不调用 generator/parser/commands/cupdate 函数，故无需 mock。
 */
#include "test.h"
#include "utils/utils.c"
#include "domain/core/core.c"
#include "domain/domain.c"
#include "typecheck/typecheck.c"
#include "docgen/docgen.c"

#include <unistd.h>

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

#define ASSERT_CONTAINS(c, n, m)    ASSERT_TRUE(contains(c, n), m)
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

#define TMP_FILE "/tmp/test_docgen_tmp.txt"

/* 打开临时写入文件 */
static FILE *open_tmp(void) {
    return fopen(TMP_FILE, "w");
}

/* 关闭并读取临时文件内容（调用者 free） */
static char *close_and_read(FILE *f) {
    fclose(f);
    return read_file(TMP_FILE);
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
/* 1. docgen_is_api (static)                                          */
/* ================================================================== */
TEST_SUITE(test_docgen_is_api) {
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
    ModuleDomain *mod = domain_module_domain_new("mod");
    VariableDomain *v = domain_variable_domain_new("v", "int");
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    ASSERT_EQ_INT(docgen_is_api((Domain *)mod), 0, "MODULE 非 API");
    ASSERT_EQ_INT(docgen_is_api((Domain *)v), 0, "VARIABLE 非 API");
    ASSERT_EQ_INT(docgen_is_api((Domain *)mb), 0, "MEMBER 非 API");
    domain_domain_delete((Domain *)mod);
    domain_domain_delete((Domain *)v);
    domain_domain_delete((Domain *)mb);
    ENDCASE;
}

/* ================================================================== */
/* 2. docgen_domain_type_str (static)                                 */
/* ================================================================== */
TEST_SUITE(test_docgen_domain_type_str) {
    CASE("domain_type_str: 全部分支");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_FUNCTION), "函数", "FUNCTION");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_STRUCT), "结构体", "STRUCT");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_TYPE), "类型", "TYPE");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_MACRO), "宏", "MACRO");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_MEMBER), "成员", "MEMBER");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_VARIABLE), "变量", "VARIABLE");
    ASSERT_EQ_STR(docgen_domain_type_str(DOMAIN_MODULE), "未知", "MODULE 走 default");
    ASSERT_EQ_STR(docgen_domain_type_str(999), "未知", "非法值走 default");
    ENDCASE;
}

/* ================================================================== */
/* 3. docgen_is_item (static)                                         */
/* ================================================================== */
TEST_SUITE(test_docgen_is_item) {
    CASE("is_item: 可枚举项返回 1");
    FunctionDomain *f = domain_function_domain_new("f", "void");
    StructDomain *s = domain_struct_domain_new("S");
    TypeDomain *t = domain_type_domain_new("T");
    MacroDomain *m = domain_macro_domain_new("M");
    ASSERT_EQ_INT(docgen_is_item((Domain *)f), 1, "FUNCTION 是 item");
    ASSERT_EQ_INT(docgen_is_item((Domain *)s), 1, "STRUCT 是 item");
    ASSERT_EQ_INT(docgen_is_item((Domain *)t), 1, "TYPE 是 item");
    ASSERT_EQ_INT(docgen_is_item((Domain *)m), 1, "MACRO 是 item");
    domain_domain_delete((Domain *)f);
    domain_domain_delete((Domain *)s);
    domain_domain_delete((Domain *)t);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("is_item: 非枚举项返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    VariableDomain *v = domain_variable_domain_new("v", "int");
    MemberDomain *mb = domain_member_domain_new("mb", "int");
    ASSERT_EQ_INT(docgen_is_item((Domain *)mod), 0, "MODULE 非 item");
    ASSERT_EQ_INT(docgen_is_item((Domain *)v), 0, "VARIABLE 非 item");
    ASSERT_EQ_INT(docgen_is_item((Domain *)mb), 0, "MEMBER 非 item");
    domain_domain_delete((Domain *)mod);
    domain_domain_delete((Domain *)v);
    domain_domain_delete((Domain *)mb);
    ENDCASE;
}

/* ================================================================== */
/* 4. docgen_generate_docs (public)                                   */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_docs) {
    CASE("generate_docs: NULL proj 返回 -1");
    ASSERT_EQ_INT(docgen_generate_docs(NULL, "/tmp/test_docgen_null"), -1, "NULL 应返回 -1");
    ENDCASE;

    CASE("generate_docs: 有效 proj 生成三份文档");
    const char *dir = "/tmp/test_docgen_full";
    make_tmp_dir(dir);
    Project *p = domain_project_new("myproj");
    ModuleDomain *mod = domain_module_domain_new("utils");
    domain_domain_add_child(p->root, (Domain *)mod);

    ASSERT_EQ_INT(docgen_generate_docs(p, dir), 0, "应返回 0");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_full/README.md"), "应生成 README.md");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_full/DEV.md"), "应生成 DEV.md");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_full/DEPENDENCIES.md"), "应生成 DEPENDENCIES.md");

    char *readme = read_file("/tmp/test_docgen_full/README.md");
    ASSERT_NOT_NULL(readme, "读取 README");
    ASSERT_CONTAINS(readme, "# myproj", "README 标题");
    ASSERT_CONTAINS(readme, "*Generated by CBoot v", "README 版本签名");
    free(readme);

    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_full");
    ENDCASE;
}

/* ================================================================== */
/* 5. docgen_generate_module_docs (public)                            */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_module_docs) {
    CASE("generate_module_docs: NULL 不崩溃");
    docgen_generate_module_docs(NULL, "/tmp/test_docgen_mn");
    ASSERT_TRUE(1, "NULL 安全返回");
    ENDCASE;

    CASE("generate_module_docs: 非 module 不生成文件");
    const char *dir = "/tmp/test_docgen_mnm";
    make_tmp_dir(dir);
    FunctionDomain *f = domain_function_domain_new("f", "void");
    docgen_generate_module_docs((Domain *)f, dir);
    ASSERT_FALSE(utils_file_exists("/tmp/test_docgen_mnm/README.md"), "非 module 不应生成 README");
    domain_domain_delete((Domain *)f);
    system("rm -rf /tmp/test_docgen_mnm");
    ENDCASE;

    CASE("generate_module_docs: 有效 module 生成 README/API/DEV");
    const char *dir2 = "/tmp/test_docgen_mm";
    make_tmp_dir(dir2);
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fn = domain_function_domain_new("do_it", "int");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fn);

    docgen_generate_module_docs((Domain *)mod, dir2);
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_mm/README.md"), "生成模块 README");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_mm/API.md"), "生成模块 API");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_mm/DEV.md"), "生成模块 DEV");

    char *api = read_file("/tmp/test_docgen_mm/API.md");
    ASSERT_CONTAINS(api, "# mod API 文档", "API 标题");
    free(api);

    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_mm");
    ENDCASE;

    CASE("generate_module_docs: 递归子模块");
    const char *dir3 = "/tmp/test_docgen_mr";
    make_tmp_dir(dir3);
    make_tmp_dir("/tmp/test_docgen_mr/sub");  /* docgen 自身不建子目录，按契约预建 */
    ModuleDomain *mod = domain_module_domain_new("root_mod");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)sub);

    docgen_generate_module_docs((Domain *)mod, dir3);
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_mr/sub/README.md"), "递归生成子模块 README");
    ASSERT_TRUE(utils_file_exists("/tmp/test_docgen_mr/sub/API.md"), "递归生成子模块 API");

    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_mr");
    ENDCASE;
}

/* ================================================================== */
/* 6. docgen_write_api_deps (static)                                  */
/* ================================================================== */
TEST_SUITE(test_docgen_write_api_deps) {
    CASE("write_api_deps: 0 依赖");
    Project *p = domain_project_new("p");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_api_deps(p, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "无 API 依赖。", "0 依赖提示");
    ASSERT_NOT_CONTAINS(c, "依赖链", "0 依赖无链");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("write_api_deps: 有依赖（含/不含 cboot_file）");
    Project *p = domain_project_new("p");
    domain_project_add_dependency(p, "/a", "/b", "b.cboot");
    domain_project_add_dependency(p, "/c", "/d", NULL);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_api_deps(p, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "## API 依赖 (im 导入)", "标题");
    ASSERT_CONTAINS(c, "| `/a` | `/b` | b.cboot |", "依赖视图行(含 cboot)");
    ASSERT_CONTAINS(c, "| `/c` | `/d` | - |", "依赖视图行(无 cboot)");
    ASSERT_CONTAINS(c, "### 被依赖视图", "被依赖视图段");
    ASSERT_CONTAINS(c, "/a --> /b  (b.cboot)", "依赖链(含 cboot)");
    ASSERT_CONTAINS(c, "/c --> /d", "依赖链(无 cboot)");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 7. docgen_generate_dependencies_doc (static)                       */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_dependencies_doc) {
    CASE("generate_dependencies_doc: 仅 im 依赖");
    const char *dir = "/tmp/test_docgen_dep1";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    domain_project_add_dependency(p, "/x", "/y", "y.cboot");
    docgen_generate_dependencies_doc(p, dir);
    char *c = read_file("/tmp/test_docgen_dep1/DEPENDENCIES.md");
    ASSERT_NOT_NULL(c, "读取 DEPENDENCIES");
    ASSERT_CONTAINS(c, "# proj 依赖关系", "标题");
    ASSERT_CONTAINS(c, "/x --> /y", "im 依赖链");
    ASSERT_CONTAINS(c, "无完整项目导入。", "无 in 导入提示");
    ASSERT_CONTAINS(c, "*Generated by CBoot v", "版本签名");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_dep1");
    ENDCASE;

    CASE("generate_dependencies_doc: 含 in 导入");
    const char *dir = "/tmp/test_docgen_dep2";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    domain_project_add_dependency(p, "/x", "/y", NULL);
    proj_add_import(p, "libfoo.cboot");
    proj_add_import(p, "libbar.cboot");
    docgen_generate_dependencies_doc(p, dir);
    char *c = read_file("/tmp/test_docgen_dep2/DEPENDENCIES.md");
    ASSERT_NOT_NULL(c, "读取 DEPENDENCIES");
    ASSERT_CONTAINS(c, "## 完整项目导入 (in 导入)", "in 段标题");
    ASSERT_CONTAINS(c, "| 1 | libfoo.cboot |", "in 导入第1行");
    ASSERT_CONTAINS(c, "| 2 | libbar.cboot |", "in 导入第2行");
    ASSERT_NOT_CONTAINS(c, "无完整项目导入。", "有 in 不应出现无导入提示");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_dep2");
    ENDCASE;

    CASE("generate_dependencies_doc: 无任何依赖");
    const char *dir = "/tmp/test_docgen_dep3";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    docgen_generate_dependencies_doc(p, dir);
    char *c = read_file("/tmp/test_docgen_dep3/DEPENDENCIES.md");
    ASSERT_NOT_NULL(c, "读取 DEPENDENCIES");
    ASSERT_CONTAINS(c, "无 API 依赖。", "无 im 提示");
    ASSERT_CONTAINS(c, "无完整项目导入。", "无 in 提示");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_dep3");
    ENDCASE;
}

/* ================================================================== */
/* 8. docgen_proj_readme_write_modules (static)                       */
/* ================================================================== */
TEST_SUITE(test_docgen_proj_readme_write_modules) {
    CASE("write_modules: 有/无注释的模块");
    Project *p = domain_project_new("p");
    ModuleDomain *m1 = domain_module_domain_new("m1");
    ModuleDomain *m2 = domain_module_domain_new("m2");
    domain_domain_set_comment((Domain *)m2, "second module");
    domain_domain_add_child(p->root, (Domain *)m1);
    domain_domain_add_child(p->root, (Domain *)m2);
    /* 加一个非 module 子节点，应被跳过 */
    FunctionDomain *fn = domain_function_domain_new("skip", "void");
    domain_domain_add_child(p->root, (Domain *)fn);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_modules(p, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "## 模块", "章节标题");
    ASSERT_CONTAINS(c, "- [m1](m1/README.md)", "无注释模块行");
    ASSERT_CONTAINS(c, "- [m2](m2/README.md): second module", "带注释模块行");
    ASSERT_NOT_CONTAINS(c, "skip", "非 module 被跳过");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 9. docgen_proj_readme_write_mod_entry (static) - 各 mode           */
/* ================================================================== */
TEST_SUITE(test_docgen_proj_readme_write_mod_entry) {
    CASE("write_mod_entry: EXTERNAL");
    ModuleDomain *m = domain_module_domain_new("ext");
    m->mode = MOD_MODE_EXTERNAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_mod_entry((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "├── ext/", "external 顶层行");
    ASSERT_CONTAINS(c, "│   ├── ext.h", "external 头文件");
    ASSERT_CONTAINS(c, "│   ├── .cboot", "cboot 行");
    ASSERT_CONTAINS(c, "│   ├── README.md", "README 行");
    ASSERT_NOT_CONTAINS(c, "API.md", "external 无 API.md");
    ASSERT_NOT_CONTAINS(c, "CMakeLists.txt", "external 无 CMake");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("write_mod_entry: STATIC 带 lib 路径");
    ModuleDomain *m = domain_module_domain_new("stat");
    m->mode = MOD_MODE_STATIC;
    domain_domain_set_value((Domain *)m, "/usr/lib/libfoo.a");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_mod_entry((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "(mode: static, lib: /usr/lib/libfoo.a)", "static lib 标注");
    ASSERT_CONTAINS(c, "│   ├── stat.h", "static 头文件");
    ASSERT_CONTAINS(c, "│   ├── API.md", "static 有 API.md");
    ASSERT_CONTAINS(c, "│   └── CMakeLists.txt", "static 有 CMake");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("write_mod_entry: DYNAMIC 带 lib 路径");
    ModuleDomain *m = domain_module_domain_new("dyn");
    m->mode = MOD_MODE_DYNAMIC;
    domain_domain_set_value((Domain *)m, "/usr/lib/libbar.so");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_mod_entry((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "(mode: dynamic, lib: /usr/lib/libbar.so)", "dynamic lib 标注");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("write_mod_entry: SRC + COMPILER_EXE");
    ModuleDomain *m = domain_module_domain_new("exe");
    m->mode = MOD_MODE_SRC;
    m->compiler = COMPILER_EXE;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_mod_entry((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "│   ├── main.c", "exe 有 main.c");
    ASSERT_CONTAINS(c, "│   ├── exe.h", "exe 有头文件");
    ASSERT_NOT_CONTAINS(c, "│   ├── exe.c", "exe 不应有 exe.c");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;

    CASE("write_mod_entry: SRC + COMPILER_NORMAL");
    ModuleDomain *m = domain_module_domain_new("src");
    m->mode = MOD_MODE_SRC;
    m->compiler = COMPILER_NORMAL;
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_mod_entry((Domain *)m, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "│   ├── src.c", "normal 有 src.c");
    ASSERT_CONTAINS(c, "│   ├── src.h", "normal 有头文件");
    ASSERT_NOT_CONTAINS(c, "│   ├── main.c", "normal 不应有 main.c");
    free(c);
    domain_domain_delete((Domain *)m);
    ENDCASE;
}

/* ================================================================== */
/* 10. docgen_proj_readme_write_tree (static)                         */
/* ================================================================== */
TEST_SUITE(test_docgen_proj_readme_write_tree) {
    CASE("write_tree: 含模块的目录结构");
    Project *p = domain_project_new("proj");
    ModuleDomain *m = domain_module_domain_new("mod");
    domain_domain_add_child(p->root, (Domain *)m);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_proj_readme_write_tree(p, f);
    char *c = close_and_read(f);
    ASSERT_NOT_NULL(c, "读取内容");
    ASSERT_CONTAINS(c, "## 目录结构", "章节标题");
    ASSERT_CONTAINS(c, "proj/", "项目根");
    ASSERT_CONTAINS(c, "├── CMakeLists.txt", "顶层 CMake");
    ASSERT_CONTAINS(c, "├── DEPENDENCIES.md", "顶层 DEPENDENCIES");
    ASSERT_CONTAINS(c, "├── mod/", "模块条目");
    ASSERT_CONTAINS(c, "└── .cboot", "结尾 .cboot");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 11. docgen_generate_project_readme (static)                        */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_project_readme) {
    CASE("generate_project_readme: 含注释");
    const char *dir = "/tmp/test_docgen_pr1";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    domain_domain_set_comment(p->root, "项目说明文字");
    ModuleDomain *m = domain_module_domain_new("mod");
    domain_domain_add_child(p->root, (Domain *)m);
    docgen_generate_project_readme(p, dir);
    char *c = read_file("/tmp/test_docgen_pr1/README.md");
    ASSERT_NOT_NULL(c, "读取 README");
    ASSERT_CONTAINS(c, "# proj", "标题");
    ASSERT_CONTAINS(c, "项目说明文字", "根注释");
    ASSERT_CONTAINS(c, "## 文档", "文档段");
    ASSERT_CONTAINS(c, "[开发文档](DEV.md)", "DEV 链接");
    ASSERT_CONTAINS(c, "[依赖关系](DEPENDENCIES.md)", "DEPENDENCIES 链接");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_pr1");
    ENDCASE;

    CASE("generate_project_readme: 无注释");
    const char *dir = "/tmp/test_docgen_pr2";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    docgen_generate_project_readme(p, dir);
    char *c = read_file("/tmp/test_docgen_pr2/README.md");
    ASSERT_NOT_NULL(c, "读取 README");
    ASSERT_CONTAINS(c, "# proj", "标题");
    ASSERT_NOT_CONTAINS(c, "项目说明文字", "无注释不应出现");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_pr2");
    ENDCASE;
}

/* ================================================================== */
/* 12. docgen_dev_write_funcs / types / macros (static)               */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_funcs_types_macros) {
    CASE("dev_write_funcs: 函数含注释与逻辑");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fn = domain_function_domain_new("add", "int");
    domain_domain_set_comment((Domain *)fn, "相加");
    domain_domain_set_value((Domain *)fn, "return a+b;");
    domain_domain_add_child((Domain *)mod, (Domain *)fn);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_funcs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**函数:**", "函数段标题");
    ASSERT_CONTAINS(c, "- `int add()`", "函数签名行");
    ASSERT_CONTAINS(c, " — 相加", "注释");
    ASSERT_CONTAINS(c, "  \n  逻辑: return a+b;", "业务逻辑");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("dev_write_funcs: 无函数不输出段");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_dev_write_funcs((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "**函数:**", "无函数不应有段标题");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("dev_write_types: 结构体/类型含注释");
    ModuleDomain *mod = domain_module_domain_new("mod");
    StructDomain *s = domain_struct_domain_new("S");
    domain_domain_set_comment((Domain *)s, "结构");
    TypeDomain *t = domain_type_domain_new("T");
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**类型:**", "类型段标题");
    ASSERT_CONTAINS(c, "- `S` — 结构", "结构体行");
    ASSERT_CONTAINS(c, "- `T`", "类型行");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("dev_write_macros: 宏含值与注释");
    ModuleDomain *mod = domain_module_domain_new("mod");
    MacroDomain *m = domain_macro_domain_new("MAX");
    domain_domain_set_value((Domain *)m, "100");
    domain_domain_set_comment((Domain *)m, "上限");
    domain_domain_add_child((Domain *)mod, (Domain *)m);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**宏:**", "宏段标题");
    ASSERT_CONTAINS(c, "- `MAX` = `100` — 上限", "宏行");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 13. docgen_dev_write_deps (static)                                 */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_deps) {
    CASE("dev_write_deps: 无依赖不输出");
    Project *p = domain_project_new("p");
    FILE *f = open_tmp();
    docgen_dev_write_deps(p, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 依赖链", "无依赖无段");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("dev_write_deps: 仅 im 依赖");
    Project *p = domain_project_new("p");
    domain_project_add_dependency(p, "/a", "/b", "b.cboot");
    FILE *f = open_tmp();
    docgen_dev_write_deps(p, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 依赖链", "依赖链段");
    ASSERT_CONTAINS(c, "### API 依赖摘要", "摘要段");
    ASSERT_CONTAINS(c, "/a --> /b", "链行");
    free(c);
    domain_project_free(p);
    ENDCASE;

    CASE("dev_write_deps: 仅 in 导入（无 im 摘要）");
    Project *p = domain_project_new("p");
    proj_add_import(p, "lib.cboot");
    FILE *f = open_tmp();
    docgen_dev_write_deps(p, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 依赖链", "有 in 仍输出依赖链段");
    ASSERT_NOT_CONTAINS(c, "### API 依赖摘要", "无 im 不应有摘要");
    free(c);
    domain_project_free(p);
    ENDCASE;
}

/* ================================================================== */
/* 14. docgen_generate_project_dev (static)                           */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_project_dev) {
    CASE("generate_project_dev: 模块概览与详细");
    const char *dir = "/tmp/test_docgen_pd";
    make_tmp_dir(dir);
    Project *p = domain_project_new("proj");
    domain_domain_set_comment(p->root, "项目说明");
    ModuleDomain *m = domain_module_domain_new("mod");
    domain_domain_set_comment((Domain *)m, "模块说明");
    FunctionDomain *fn = domain_function_domain_new("fn", "void");
    domain_domain_add_child((Domain *)m, (Domain *)fn);
    domain_domain_add_child(p->root, (Domain *)m);

    docgen_generate_project_dev(p, dir);
    char *c = read_file("/tmp/test_docgen_pd/DEV.md");
    ASSERT_NOT_NULL(c, "读取 DEV");
    ASSERT_CONTAINS(c, "# proj 开发文档", "标题");
    ASSERT_CONTAINS(c, "项目说明", "根注释");
    ASSERT_CONTAINS(c, "## 模块概览", "概览段");
    ASSERT_CONTAINS(c, "| `mod` | 模块说明 | mod/ |", "概览表行");
    ASSERT_CONTAINS(c, "### mod", "详细段");
    ASSERT_CONTAINS(c, "详细文档见 [mod](mod/DEV.md)。", "模块链接");
    ASSERT_CONTAINS(c, "*Generated by CBoot v", "版本签名");
    free(c);
    domain_project_free(p);
    system("rm -rf /tmp/test_docgen_pd");
    ENDCASE;
}

/* ================================================================== */
/* 15. docgen_readme_write_submodules (static)                        */
/* ================================================================== */
TEST_SUITE(test_docgen_readme_write_submodules) {
    CASE("readme_write_submodules: 有/无注释子模块");
    ModuleDomain *mod = domain_module_domain_new("mod");
    ModuleDomain *s1 = domain_module_domain_new("s1");
    ModuleDomain *s2 = domain_module_domain_new("s2");
    domain_domain_set_comment((Domain *)s2, "sub two");
    domain_domain_add_child((Domain *)mod, (Domain *)s1);
    domain_domain_add_child((Domain *)mod, (Domain *)s2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_readme_write_submodules((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 子模块", "子模块段");
    ASSERT_CONTAINS(c, "- [s1](s1/README.md)", "无注释子模块");
    ASSERT_CONTAINS(c, "- [s2](s2/README.md): sub two", "带注释子模块");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("readme_write_submodules: 无子模块不输出");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_readme_write_submodules((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 子模块", "无子模块无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 16. docgen_readme_write_item_summary (static)                      */
/* ================================================================== */
TEST_SUITE(test_docgen_readme_write_item_summary) {
    CASE("write_item_summary: 含 API 与私有项");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("api_fn", "int");
    fa->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)fa, "公开");
    FunctionDomain *fp = domain_function_domain_new("priv_fn", "void");
    /* fp 保持 NORMAL */
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_readme_write_item_summary((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 内容概览", "概览段");
    ASSERT_CONTAINS(c, "| 名称 | 类型 | API | 说明 |", "表头");
    ASSERT_CONTAINS(c, "| `api_fn` | 函数 | ✓ | 公开 |", "API 行");
    ASSERT_CONTAINS(c, "| `priv_fn` | 函数 | — | - |", "私有行");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("write_item_summary: 无内容项不输出");
    ModuleDomain *mod = domain_module_domain_new("mod");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)sub);  /* 非 item */
    FILE *f = open_tmp();
    docgen_readme_write_item_summary((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 内容概览", "无 item 无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 17. docgen_generate_mod_readme (static)                            */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_mod_readme) {
    CASE("generate_mod_readme: 含注释/子模块/概览");
    const char *dir = "/tmp/test_docgen_mr1";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    domain_domain_set_comment((Domain *)mod, "模块说明");
    ModuleDomain *sub = domain_module_domain_new("sub");
    FunctionDomain *fn = domain_function_domain_new("api_fn", "int");
    fn->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)sub);
    domain_domain_add_child((Domain *)mod, (Domain *)fn);

    docgen_generate_mod_readme((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_mr1/README.md");
    ASSERT_NOT_NULL(c, "读取 README");
    ASSERT_CONTAINS(c, "# mod", "标题");
    ASSERT_CONTAINS(c, "模块说明", "模块注释");
    ASSERT_CONTAINS(c, "- [API 文档](API.md)", "API 链接");
    ASSERT_CONTAINS(c, "- [开发文档](DEV.md)", "DEV 链接");
    ASSERT_CONTAINS(c, "- [sub](sub/README.md)", "子模块链接");
    ASSERT_CONTAINS(c, "| `api_fn`", "内容概览");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_mr1");
    ENDCASE;

    CASE("generate_mod_readme: 无注释");
    const char *dir = "/tmp/test_docgen_mr2";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    docgen_generate_mod_readme((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_mr2/README.md");
    ASSERT_CONTAINS(c, "# mod", "标题");
    ASSERT_NOT_CONTAINS(c, "模块说明", "无注释不应出现");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_mr2");
    ENDCASE;
}

/* ================================================================== */
/* 18. docgen_api_write_toc (static)                                  */
/* ================================================================== */
TEST_SUITE(test_docgen_api_write_toc) {
    CASE("api_write_toc: 有 API 项");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("api_fn", "int");
    fa->mode = API_MODE_API;
    FunctionDomain *fp = domain_function_domain_new("priv_fn", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_api_write_toc((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "## 目录", "目录段");
    ASSERT_CONTAINS(c, "- [api_fn](#api_fn)", "API 条目");
    ASSERT_NOT_CONTAINS(c, "[priv_fn]", "私有项不入目录");
    ASSERT_CONTAINS(c, "\n---\n", "分隔符");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("api_write_toc: 无 API 项返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fp = domain_function_domain_new("priv_fn", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fp);
    FILE *f = open_tmp();
    int r = docgen_api_write_toc((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    ASSERT_NOT_CONTAINS(c, "## 目录", "无 API 无目录");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 19. docgen_write_func_signature (static, API 版)                   */
/* ================================================================== */
TEST_SUITE(test_docgen_write_func_signature) {
    CASE("write_func_signature: 多参数");
    FunctionDomain *fn = domain_function_domain_new("add", "int");
    MemberDomain *p1 = domain_member_domain_new("a", "int");
    MemberDomain *p2 = domain_member_domain_new("b", "int");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    domain_domain_add_child((Domain *)fn, (Domain *)p2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_func_signature(fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int add(int a, int b)", "签名含参数");
    ASSERT_CONTAINS(c, "```c", "代码块开始");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_func_signature: 无参数");
    FunctionDomain *fn = domain_function_domain_new("get", "void");
    FILE *f = open_tmp();
    docgen_write_func_signature(fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void get()", "无参数签名");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 20. docgen_write_func_params (static, API 版, 返回 int)            */
/* ================================================================== */
TEST_SUITE(test_docgen_write_func_params) {
    CASE("write_func_params: 有参数返回 1");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    MemberDomain *p1 = domain_member_domain_new("x", "int");
    domain_domain_set_comment((Domain *)p1, "坐标");
    MemberDomain *p2 = domain_member_domain_new("y", "int");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    domain_domain_add_child((Domain *)fn, (Domain *)p2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_write_func_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "**参数**:", "参数段标题");
    ASSERT_CONTAINS(c, "| `x` | `int` | 坐标 |", "带注释参数行");
    ASSERT_CONTAINS(c, "| `y` | `int` | - |", "无注释参数行");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("write_func_params: 无参数返回 0");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    FILE *f = open_tmp();
    int r = docgen_write_func_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    ASSERT_NOT_CONTAINS(c, "**参数**:", "无参数无段");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 21. docgen_api_write_functions (static)                            */
/* ================================================================== */
TEST_SUITE(test_docgen_api_write_functions) {
    CASE("api_write_functions: API 函数含签名与参数");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("open", "FILE*");
    fa->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)fa, "打开文件");
    MemberDomain *p = domain_member_domain_new("path", "const char*");
    domain_domain_add_child((Domain *)fa, (Domain *)p);
    FunctionDomain *fp = domain_function_domain_new("helper", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_api_write_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "## 函数", "函数段");
    ASSERT_CONTAINS(c, "### FILE* open()", "函数小标题");
    ASSERT_CONTAINS(c, "打开文件", "注释");
    ASSERT_CONTAINS(c, "FILE* open(const char* path)", "签名");
    ASSERT_CONTAINS(c, "**参数**:", "参数段");
    ASSERT_NOT_CONTAINS(c, "helper", "私有函数不出现");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("api_write_functions: 无 API 函数返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fp = domain_function_domain_new("helper", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fp);
    FILE *f = open_tmp();
    int r = docgen_api_write_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    ASSERT_NOT_CONTAINS(c, "## 函数", "无 API 函数无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 22. docgen_api_write_types (static)                                */
/* ================================================================== */
TEST_SUITE(test_docgen_api_write_types) {
    CASE("api_write_types: API 结构体与类型");
    ModuleDomain *mod = domain_module_domain_new("mod");
    StructDomain *s = domain_struct_domain_new("Point");
    s->mode = API_MODE_API;
    MemberDomain *mx = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    TypeDomain *t = domain_type_domain_new("Handle");
    t->mode = TYPE_MODE_API_RENAME;
    domain_domain_set_value((Domain *)t, "void*");
    StructDomain *sp = domain_struct_domain_new("Priv");
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    domain_domain_add_child((Domain *)mod, (Domain *)t);
    domain_domain_add_child((Domain *)mod, (Domain *)sp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_api_write_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "## 类型", "类型段");
    ASSERT_CONTAINS(c, "### Point", "结构体小标题");
    ASSERT_CONTAINS(c, "typedef struct Point {", "结构体定义");
    ASSERT_CONTAINS(c, "### Handle", "类型小标题");
    ASSERT_CONTAINS(c, "typedef void* Handle;", "类型定义");
    ASSERT_NOT_CONTAINS(c, "Priv", "私有结构体不出现");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("api_write_types: 无 API 类型返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    StructDomain *sp = domain_struct_domain_new("Priv");
    domain_domain_add_child((Domain *)mod, (Domain *)sp);
    FILE *f = open_tmp();
    int r = docgen_api_write_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 23. docgen_api_write_macros (static)                               */
/* ================================================================== */
TEST_SUITE(test_docgen_api_write_macros) {
    CASE("api_write_macros: API 宏含值与注释");
    ModuleDomain *mod = domain_module_domain_new("mod");
    MacroDomain *ma = domain_macro_domain_new("MAX_SIZE");
    ma->mode = API_MODE_API;
    domain_domain_set_value((Domain *)ma, "256");
    domain_domain_set_comment((Domain *)ma, "最大尺寸");
    MacroDomain *mp = domain_macro_domain_new("INTERNAL");
    domain_domain_add_child((Domain *)mod, (Domain *)ma);
    domain_domain_add_child((Domain *)mod, (Domain *)mp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_api_write_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "## 宏", "宏段");
    ASSERT_CONTAINS(c, "### `MAX_SIZE`", "宏小标题");
    ASSERT_CONTAINS(c, "最大尺寸", "注释");
    ASSERT_CONTAINS(c, "#define MAX_SIZE 256", "宏定义");
    ASSERT_NOT_CONTAINS(c, "INTERNAL", "私有宏不出现");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("api_write_macros: 无 API 宏返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    MacroDomain *mp = domain_macro_domain_new("INTERNAL");
    domain_domain_add_child((Domain *)mod, (Domain *)mp);
    FILE *f = open_tmp();
    int r = docgen_api_write_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 24. docgen_api_write_submodules (static)                           */
/* ================================================================== */
TEST_SUITE(test_docgen_api_write_submodules) {
    CASE("api_write_submodules: 子模块链接");
    ModuleDomain *mod = domain_module_domain_new("mod");
    ModuleDomain *s = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)s);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    int r = docgen_api_write_submodules((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 1, "应返回 1");
    ASSERT_CONTAINS(c, "## 子模块", "子模块段");
    ASSERT_CONTAINS(c, "- [sub API](sub/API.md)", "子模块 API 链接");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("api_write_submodules: 无子模块返回 0");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    int r = docgen_api_write_submodules((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_EQ_INT(r, 0, "应返回 0");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 25. docgen_generate_mod_api (static)                               */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_mod_api) {
    CASE("generate_mod_api: 有 API");
    const char *dir = "/tmp/test_docgen_ma1";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("init", "int");
    fa->mode = API_MODE_API;
    domain_domain_add_child((Domain *)mod, (Domain *)fa);

    docgen_generate_mod_api((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_ma1/API.md");
    ASSERT_NOT_NULL(c, "读取 API");
    ASSERT_CONTAINS(c, "# mod API 文档", "标题");
    ASSERT_CONTAINS(c, "## 目录", "有目录");
    ASSERT_CONTAINS(c, "## 函数", "有函数段");
    ASSERT_NOT_CONTAINS(c, "此模块没有公开 API", "有 API 不应出现提示");
    ASSERT_CONTAINS(c, "*Generated by CBoot v", "版本签名");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_ma1");
    ENDCASE;

    CASE("generate_mod_api: 无 API 提示");
    const char *dir = "/tmp/test_docgen_ma2";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fp = domain_function_domain_new("priv", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fp);

    docgen_generate_mod_api((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_ma2/API.md");
    ASSERT_CONTAINS(c, "> 此模块没有公开 API。", "无 API 提示");
    ASSERT_NOT_CONTAINS(c, "## 函数", "无函数段");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_ma2");
    ENDCASE;
}

/* ================================================================== */
/* 26. docgen_dev_write_signature (static, DEV 版)                    */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_signature) {
    CASE("dev_write_signature: 多参数");
    FunctionDomain *fn = domain_function_domain_new("add", "int");
    MemberDomain *p1 = domain_member_domain_new("a", "int");
    MemberDomain *p2 = domain_member_domain_new("b", "int");
    domain_domain_add_child((Domain *)fn, (Domain *)p1);
    domain_domain_add_child((Domain *)fn, (Domain *)p2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_signature(fn, (Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "int add(int a, int b)", "DEV 签名含参数");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("dev_write_signature: 无参数");
    FunctionDomain *fn = domain_function_domain_new("get", "void");
    FILE *f = open_tmp();
    docgen_dev_write_signature(fn, (Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "void get()", "DEV 无参数签名");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 27. docgen_dev_write_params (static, DEV 版)                       */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_params) {
    CASE("dev_write_params: 有参数");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    MemberDomain *p = domain_member_domain_new("x", "int");
    domain_domain_set_comment((Domain *)p, "说明");
    domain_domain_add_child((Domain *)fn, (Domain *)p);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**参数列表**:", "DEV 参数段标题");
    ASSERT_CONTAINS(c, "| `x` | `int` | 说明 |", "参数行");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("dev_write_params: 无参数不输出");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    FILE *f = open_tmp();
    docgen_dev_write_params((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "**参数列表**:", "无参数无段");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 28. docgen_dev_write_locals (static)                               */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_locals) {
    CASE("dev_write_locals: 局部变量含注释");
    FunctionDomain *fn = domain_function_domain_new("f", "int");
    VariableDomain *v1 = domain_variable_domain_new("count", "int");
    VariableDomain *v2 = domain_variable_domain_new("buf", "char*");
    domain_domain_set_comment((Domain *)v2, "缓冲区");
    domain_domain_add_child((Domain *)fn, (Domain *)v1);
    domain_domain_add_child((Domain *)fn, (Domain *)v2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_locals((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**局部变量**:", "局部变量段");
    ASSERT_CONTAINS(c, "- `count` (`int`)", "无注释变量");
    ASSERT_CONTAINS(c, "- `buf` (`char*`): 缓冲区", "带注释变量");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;

    CASE("dev_write_locals: 无变量不输出");
    FunctionDomain *fn = domain_function_domain_new("f", "void");
    FILE *f = open_tmp();
    docgen_dev_write_locals((Domain *)fn, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "**局部变量**:", "无变量无段");
    free(c);
    domain_domain_delete((Domain *)fn);
    ENDCASE;
}

/* ================================================================== */
/* 29. docgen_write_dev_functions (static)                            */
/* ================================================================== */
TEST_SUITE(test_docgen_write_dev_functions) {
    CASE("write_dev_functions: API/私有函数及附属");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("init", "int");
    fa->mode = API_MODE_API;
    domain_domain_set_value((Domain *)fa, "setup state");
    domain_domain_set_comment((Domain *)fa, "初始化");
    MemberDomain *p = domain_member_domain_new("cfg", "void*");
    VariableDomain *loc = domain_variable_domain_new("rc", "int");
    domain_domain_add_child((Domain *)fa, (Domain *)p);
    domain_domain_add_child((Domain *)fa, (Domain *)loc);
    FunctionDomain *fp = domain_function_domain_new("helper", "void");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_dev_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 函数", "函数段");
    ASSERT_CONTAINS(c, "### int init()", "API 函数小标题");
    ASSERT_CONTAINS(c, "> `API` — 公开接口", "API 标记");
    ASSERT_CONTAINS(c, "**业务逻辑**: setup state", "业务逻辑");
    ASSERT_CONTAINS(c, "**说明**: 初始化", "说明");
    ASSERT_CONTAINS(c, "int init(void* cfg)", "签名");
    ASSERT_CONTAINS(c, "**参数列表**:", "参数列表");
    ASSERT_CONTAINS(c, "**局部变量**:", "局部变量");
    ASSERT_CONTAINS(c, "### void helper()", "私有函数也出现");
    ASSERT_NOT_CONTAINS(c, "### void helper()\n\n> `API`", "私有函数无 API 标记");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("write_dev_functions: 无函数不输出段");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_write_dev_functions((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 函数", "无函数无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 30. docgen_dev_write_type_def (static)                             */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_type_def) {
    CASE("dev_write_type_def: TYPE RENAME 含值");
    TypeDomain *t = domain_type_domain_new("MyInt");
    t->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)t, "long");
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_type_def((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef long MyInt;", "rename 定义");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("dev_write_type_def: TYPE API_RENAME 无值默认 int");
    TypeDomain *t = domain_type_domain_new("T");
    t->mode = TYPE_MODE_API_RENAME;
    FILE *f = open_tmp();
    docgen_dev_write_type_def((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef int T;", "无值默认 int");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("dev_write_type_def: TYPE STRUCT 模式");
    TypeDomain *t = domain_type_domain_new("Op");
    t->mode = TYPE_MODE_STRUCT;
    FILE *f = open_tmp();
    docgen_dev_write_type_def((Domain *)t, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef struct Op { ... } Op;", "struct 占位定义");
    free(c);
    domain_domain_delete((Domain *)t);
    ENDCASE;

    CASE("dev_write_type_def: STRUCT 含成员");
    StructDomain *s = domain_struct_domain_new("Point");
    MemberDomain *mx = domain_member_domain_new("x", "int");
    MemberDomain *my = domain_member_domain_new("y", "int");
    domain_domain_add_child((Domain *)s, (Domain *)mx);
    domain_domain_add_child((Domain *)s, (Domain *)my);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_type_def((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "typedef struct Point {", "struct 开始");
    ASSERT_CONTAINS(c, "    int x;", "成员 x");
    ASSERT_CONTAINS(c, "    int y;", "成员 y");
    ASSERT_CONTAINS(c, "} Point;", "struct 结束");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;
}

/* ================================================================== */
/* 31. docgen_dev_write_members (static)                              */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_members) {
    CASE("dev_write_members: 有成员表格");
    StructDomain *s = domain_struct_domain_new("S");
    MemberDomain *m1 = domain_member_domain_new("a", "int");
    domain_domain_set_comment((Domain *)m1, "字段a");
    MemberDomain *m2 = domain_member_domain_new("b", "char*");
    domain_domain_add_child((Domain *)s, (Domain *)m1);
    domain_domain_add_child((Domain *)s, (Domain *)m2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_members((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "**成员**:", "成员段");
    ASSERT_CONTAINS(c, "| `a` | `int` | 字段a |", "成员 a 行");
    ASSERT_CONTAINS(c, "| `b` | `char*` | - |", "成员 b 行");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;

    CASE("dev_write_members: 无成员不输出");
    StructDomain *s = domain_struct_domain_new("S");
    FILE *f = open_tmp();
    docgen_dev_write_members((Domain *)s, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "**成员**:", "无成员无段");
    free(c);
    domain_domain_delete((Domain *)s);
    ENDCASE;
}

/* ================================================================== */
/* 32. docgen_write_dev_types (static)                                */
/* ================================================================== */
TEST_SUITE(test_docgen_write_dev_types) {
    CASE("write_dev_types: API/私有类型");
    ModuleDomain *mod = domain_module_domain_new("mod");
    StructDomain *sa = domain_struct_domain_new("Pub");
    sa->mode = API_MODE_API;
    domain_domain_set_comment((Domain *)sa, "公开结构");
    MemberDomain *mx = domain_member_domain_new("x", "int");
    domain_domain_add_child((Domain *)sa, (Domain *)mx);
    TypeDomain *tp = domain_type_domain_new("Priv");
    tp->mode = TYPE_MODE_RENAME;
    domain_domain_set_value((Domain *)tp, "int");
    domain_domain_add_child((Domain *)mod, (Domain *)sa);
    domain_domain_add_child((Domain *)mod, (Domain *)tp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_dev_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 类型", "类型段");
    ASSERT_CONTAINS(c, "### Pub", "API 结构小标题");
    ASSERT_CONTAINS(c, "> `API` — 公开类型", "API 标记");
    ASSERT_CONTAINS(c, "**说明**: 公开结构", "说明");
    ASSERT_CONTAINS(c, "typedef struct Pub {", "结构定义");
    ASSERT_CONTAINS(c, "**成员**:", "成员段");
    ASSERT_CONTAINS(c, "### Priv", "私有类型也出现");
    ASSERT_CONTAINS(c, "typedef int Priv;", "类型定义");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("write_dev_types: 无类型不输出段");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_write_dev_types((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 类型", "无类型无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 33. docgen_write_dev_macros (static)                               */
/* ================================================================== */
TEST_SUITE(test_docgen_write_dev_macros) {
    CASE("write_dev_macros: API/私有宏");
    ModuleDomain *mod = domain_module_domain_new("mod");
    MacroDomain *ma = domain_macro_domain_new("VERSION");
    ma->mode = API_MODE_API;
    domain_domain_set_value((Domain *)ma, "2");
    domain_domain_set_comment((Domain *)ma, "版本号");
    MacroDomain *mp = domain_macro_domain_new("DBG");
    domain_domain_set_value((Domain *)mp, "1");
    domain_domain_add_child((Domain *)mod, (Domain *)ma);
    domain_domain_add_child((Domain *)mod, (Domain *)mp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_write_dev_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 宏", "宏段");
    ASSERT_CONTAINS(c, "### `VERSION`", "API 宏小标题");
    ASSERT_CONTAINS(c, "> `API` — 公开宏", "API 标记");
    ASSERT_CONTAINS(c, "**说明**: 版本号", "说明");
    ASSERT_CONTAINS(c, "#define VERSION 2", "API 宏定义");
    ASSERT_CONTAINS(c, "### `DBG`", "私有宏小标题");
    ASSERT_CONTAINS(c, "#define DBG 1", "私有宏定义");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("write_dev_macros: 无宏不输出段");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_write_dev_macros((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 宏", "无宏无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 34. docgen_dev_write_stats (static)                                */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_stats) {
    CASE("dev_write_stats: API 与私有计数");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FunctionDomain *fa = domain_function_domain_new("a", "void");
    fa->mode = API_MODE_API;
    FunctionDomain *fb = domain_function_domain_new("b", "void");
    fb->mode = API_MODE_API;
    FunctionDomain *fp = domain_function_domain_new("p", "void");
    MacroDomain *mp = domain_macro_domain_new("M");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)fb);
    domain_domain_add_child((Domain *)mod, (Domain *)fp);
    domain_domain_add_child((Domain *)mod, (Domain *)mp);

    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_stats((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 统计", "统计段");
    ASSERT_CONTAINS(c, "- 公开 API: **2** 项", "2 个 API");
    ASSERT_CONTAINS(c, "- 私有实现: **2** 项", "2 个私有");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("dev_write_stats: 无 item 不输出");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_dev_write_stats((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 统计", "无 item 无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 35. docgen_dev_write_child_links (static)                          */
/* ================================================================== */
TEST_SUITE(test_docgen_dev_write_child_links) {
    CASE("dev_write_child_links: 子模块 DEV 链接");
    ModuleDomain *mod = domain_module_domain_new("mod");
    ModuleDomain *s1 = domain_module_domain_new("s1");
    ModuleDomain *s2 = domain_module_domain_new("s2");
    domain_domain_set_comment((Domain *)s2, "子二");
    domain_domain_add_child((Domain *)mod, (Domain *)s1);
    domain_domain_add_child((Domain *)mod, (Domain *)s2);
    FILE *f = open_tmp();
    ASSERT_NOT_NULL(f, "打开临时文件");
    docgen_dev_write_child_links((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_CONTAINS(c, "## 子模块", "子模块段");
    ASSERT_CONTAINS(c, "- [s1](s1/DEV.md)", "无注释子模块");
    ASSERT_CONTAINS(c, "- [s2](s2/DEV.md): 子二", "带注释子模块");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;

    CASE("dev_write_child_links: 无子模块不输出");
    ModuleDomain *mod = domain_module_domain_new("mod");
    FILE *f = open_tmp();
    docgen_dev_write_child_links((Domain *)mod, f);
    char *c = close_and_read(f);
    ASSERT_NOT_CONTAINS(c, "## 子模块", "无子模块无段");
    free(c);
    domain_domain_delete((Domain *)mod);
    ENDCASE;
}

/* ================================================================== */
/* 36. docgen_generate_mod_dev (static)                               */
/* ================================================================== */
TEST_SUITE(test_docgen_generate_mod_dev) {
    CASE("generate_mod_dev: 综合内容");
    const char *dir = "/tmp/test_docgen_md1";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    domain_domain_set_comment((Domain *)mod, "模块说明");
    FunctionDomain *fa = domain_function_domain_new("init", "int");
    fa->mode = API_MODE_API;
    MemberDomain *p = domain_member_domain_new("cfg", "void*");
    domain_domain_add_child((Domain *)fa, (Domain *)p);
    MacroDomain *ma = domain_macro_domain_new("MAX");
    ma->mode = API_MODE_API;
    domain_domain_set_value((Domain *)ma, "10");
    ModuleDomain *sub = domain_module_domain_new("sub");
    domain_domain_add_child((Domain *)mod, (Domain *)fa);
    domain_domain_add_child((Domain *)mod, (Domain *)ma);
    domain_domain_add_child((Domain *)mod, (Domain *)sub);

    docgen_generate_mod_dev((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_md1/DEV.md");
    ASSERT_NOT_NULL(c, "读取 DEV");
    ASSERT_CONTAINS(c, "# mod 开发文档", "标题");
    ASSERT_CONTAINS(c, "模块说明", "模块注释");
    ASSERT_CONTAINS(c, "## 统计", "统计段");
    ASSERT_CONTAINS(c, "## 函数", "函数段");
    ASSERT_CONTAINS(c, "## 宏", "宏段");
    ASSERT_CONTAINS(c, "## 子模块", "子模块段");
    ASSERT_CONTAINS(c, "- [sub](sub/DEV.md)", "子模块 DEV 链接");
    ASSERT_CONTAINS(c, "*Generated by CBoot v", "版本签名");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_md1");
    ENDCASE;

    CASE("generate_mod_dev: 无注释空模块");
    const char *dir = "/tmp/test_docgen_md2";
    make_tmp_dir(dir);
    ModuleDomain *mod = domain_module_domain_new("mod");
    docgen_generate_mod_dev((Domain *)mod, dir);
    char *c = read_file("/tmp/test_docgen_md2/DEV.md");
    ASSERT_CONTAINS(c, "# mod 开发文档", "标题");
    ASSERT_NOT_CONTAINS(c, "## 统计", "无 item 无统计段");
    ASSERT_NOT_CONTAINS(c, "## 函数", "无函数段");
    free(c);
    domain_domain_delete((Domain *)mod);
    system("rm -rf /tmp/test_docgen_md2");
    ENDCASE;
}

/* ================================================================== */
/* 主函数                                                              */
/* ================================================================== */
int main(void) {
    printf("========== docgen/docgen.c 单元测试 ==========\n");

    RUN_SUITE(test_docgen_is_api);
    RUN_SUITE(test_docgen_domain_type_str);
    RUN_SUITE(test_docgen_is_item);
    RUN_SUITE(test_docgen_generate_docs);
    RUN_SUITE(test_docgen_generate_module_docs);
    RUN_SUITE(test_docgen_write_api_deps);
    RUN_SUITE(test_docgen_generate_dependencies_doc);
    RUN_SUITE(test_docgen_proj_readme_write_modules);
    RUN_SUITE(test_docgen_proj_readme_write_mod_entry);
    RUN_SUITE(test_docgen_proj_readme_write_tree);
    RUN_SUITE(test_docgen_generate_project_readme);
    RUN_SUITE(test_docgen_dev_write_funcs_types_macros);
    RUN_SUITE(test_docgen_dev_write_deps);
    RUN_SUITE(test_docgen_generate_project_dev);
    RUN_SUITE(test_docgen_readme_write_submodules);
    RUN_SUITE(test_docgen_readme_write_item_summary);
    RUN_SUITE(test_docgen_generate_mod_readme);
    RUN_SUITE(test_docgen_api_write_toc);
    RUN_SUITE(test_docgen_write_func_signature);
    RUN_SUITE(test_docgen_write_func_params);
    RUN_SUITE(test_docgen_api_write_functions);
    RUN_SUITE(test_docgen_api_write_types);
    RUN_SUITE(test_docgen_api_write_macros);
    RUN_SUITE(test_docgen_api_write_submodules);
    RUN_SUITE(test_docgen_generate_mod_api);
    RUN_SUITE(test_docgen_dev_write_signature);
    RUN_SUITE(test_docgen_dev_write_params);
    RUN_SUITE(test_docgen_dev_write_locals);
    RUN_SUITE(test_docgen_write_dev_functions);
    RUN_SUITE(test_docgen_dev_write_type_def);
    RUN_SUITE(test_docgen_dev_write_members);
    RUN_SUITE(test_docgen_write_dev_types);
    RUN_SUITE(test_docgen_write_dev_macros);
    RUN_SUITE(test_docgen_dev_write_stats);
    RUN_SUITE(test_docgen_dev_write_child_links);
    RUN_SUITE(test_docgen_generate_mod_dev);

    /* 清理临时文件 */
    remove(TMP_FILE);

    test_summary();
    return (g_test_fail > 0) ? 1 : 0;
}
